#include "ConfigurationInputWidget.h"
#include <QComboBox>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QCheckBox>

ConfigurationInputWidget::ConfigurationInputWidget(QWidget *parent)
    : QWidget(parent), rootLayout(new QFormLayout)
{
    setLayout(rootLayout);
}

ConfigurationInputWidget::~ConfigurationInputWidget()
{
    // reap all generated widgets
    for(auto& data : fieldData) {
        if (data.label)
            delete data.label;
        Q_ASSERT(data.inputWidget);
        delete data.inputWidget;
    }
    fieldData.clear();
}

void ConfigurationInputWidget::setConfigurationDeclaration(const ConfigurationDeclaration* decl)
{
    // this function should only be called once
    Q_ASSERT(configDecl == nullptr);
    configDecl = decl;

    // populate fieldData
    int numFields = configDecl->getNumFields();
    if (numFields == 0)
        return;

    fieldData.reserve(numFields);
    for (int i = 0; i < numFields; ++i) {
        const auto& field = configDecl->getField(i);
        QWidget* inputWidget = nullptr;
        switch (field.ty) {
        case ConfigurationDeclaration::FieldType::String: {
            QLineEdit* edit = new QLineEdit(field.defaultValue);
            inputWidget = edit;
            connect(edit, &QLineEdit::textChanged, this, &ConfigurationInputWidget::refreshForm);
        }break;
        case ConfigurationDeclaration::FieldType::Integer: {
            QLineEdit* edit = new QLineEdit(field.defaultValue);
            edit->setValidator(new QIntValidator);
            inputWidget = edit;
            connect(edit, &QLineEdit::textChanged, this, &ConfigurationInputWidget::refreshForm);
        }break;
        case ConfigurationDeclaration::FieldType::FloatingPoint: {
            QLineEdit* edit = new QLineEdit(field.defaultValue);
            edit->setValidator(new QDoubleValidator);
            inputWidget = edit;
            connect(edit, &QLineEdit::textChanged, this, &ConfigurationInputWidget::refreshForm);
        }break;
        case ConfigurationDeclaration::FieldType::Enum: {
            QComboBox* cbox = new QComboBox;
            cbox->addItems(field.enumValue_DisplayValueList);
            Q_ASSERT(field.enumValue_DisplayValueList.contains(field.defaultValue));
            cbox->setCurrentText(field.defaultValue);
            inputWidget = cbox;
            connect(cbox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigurationInputWidget::refreshForm);
        }break;
        case ConfigurationDeclaration::FieldType::Boolean: {
            QCheckBox* cbox = new QCheckBox(field.displayName);
            cbox->setChecked(field.defaultValue == field.boolValue_TrueCodeValue);
            inputWidget = cbox;
            connect(cbox, &QCheckBox::stateChanged, this, &ConfigurationInputWidget::refreshForm);
        }break;
        case ConfigurationDeclaration::FieldType::Group: {
            QGroupBox* gbox = new QGroupBox(field.displayName);
            gbox->setLayout(new QFormLayout);
            inputWidget = gbox;
            // no signal-slot connection
        }break;
        }
        Q_ASSERT(inputWidget != nullptr);
        QLabel* label = nullptr;
        if (field.ty != ConfigurationDeclaration::FieldType::Group) {
            label = new QLabel(field.displayName);
        }
        int curIndex = fieldData.size();
        int parentIndex = curIndex - field.indexOffsetFromParent;
        Q_ASSERT(parentIndex >= -1 && field.indexOffsetFromParent > 0);
        FieldData data;
        data.isEnabled = false;
        data.label = label;
        data.inputWidget = inputWidget;
        data.parentLayout = (parentIndex == -1)? rootLayout : qobject_cast<QFormLayout*>(fieldData.at(parentIndex).inputWidget->layout());
        Q_ASSERT(data.parentLayout);
        fieldData.push_back(data);
    }
    Q_ASSERT(fieldData.size() == configDecl->getNumFields());
    refreshForm();
}

bool ConfigurationInputWidget::getConfigurationData(ConfigurationData& result)
{
    QHash<int,int> dummy;
    return getConfigurationData(result, dummy);
}

bool ConfigurationInputWidget::getConfigurationData(ConfigurationData& result, QHash<int,int>& fieldIndexToNodeIndexMap)
{
    if (fieldData.isEmpty()) {
        result = ConfigurationData();
        return true;
    }

    Q_ASSERT(configDecl && configDecl->getNumFields() == fieldData.size());

    TreeBuilder builder;
    QHash<int, TreeBuilder::Node*> fieldIndexToNodeMap;
    QVector<bool> enabledList(fieldData.size(), false);
    QHash<int, int> originMap; // sequence number -> field index
    // add root node
    {
        auto* root = builder.addNode(nullptr);
        originMap.insert(root->getSequenceNumber(), -1);
        fieldIndexToNodeMap.insert(-1, root);
    }

    for (int i = 0, n = fieldData.size(); i < n; ++i) {
        auto& field = fieldData.at(i);
        if (!field.isEnabled)
            continue;
        auto& decl = configDecl->getField(i);
        int parentIndex = i - decl.indexOffsetFromParent;
        if (parentIndex >= 0 && !enabledList.at(parentIndex)) {
            // parent is not enabled
            continue;
        }
        // now we are sure that this field is enabled
        enabledList[i] = true;
        TreeBuilder::Node* parent = fieldIndexToNodeMap.value(parentIndex, nullptr);
        Q_ASSERT(parent != nullptr);
        switch(decl.ty) {
        case ConfigurationDeclaration::FieldType::String:
        case ConfigurationDeclaration::FieldType::Integer:
        case ConfigurationDeclaration::FieldType::FloatingPoint:
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(field.inputWidget);
            Q_ASSERT(edit);
            if (const QValidator* validator = edit->validator()) {
                QString textCopy = edit->text();
                int pos = 0;
                if (validator->validate(textCopy, pos) != QValidator::Acceptable) {
                    return false;
                }
            }
            parent->keyList.push_back(decl.codeName);
            parent->valueList.push_back(edit->text());
        }break;
        case ConfigurationDeclaration::FieldType::Enum: {
            QComboBox* cbox = qobject_cast<QComboBox*>(field.inputWidget);
            Q_ASSERT(cbox);
            int index = cbox->currentIndex();
            Q_ASSERT(decl.enumValue_DisplayValueList.at(index) == cbox->currentText());
            parent->keyList.push_back(decl.codeName);
            parent->valueList.push_back(decl.enumValue_CodeValueList.at(index));
        }break;
        case ConfigurationDeclaration::FieldType::Boolean: {
            QCheckBox* cbox = qobject_cast<QCheckBox*>(field.inputWidget);
            Q_ASSERT(cbox);
            parent->keyList.push_back(decl.codeName);
            parent->valueList.push_back(cbox->isChecked()? decl.boolValue_TrueCodeValue: decl.boolValue_FalseCodeValue);
        }break;
        case ConfigurationDeclaration::FieldType::Group: {
            QGroupBox* gbox = qobject_cast<QGroupBox*>(field.inputWidget);
            Q_ASSERT(gbox);
            auto* ptr = builder.addNode(parent);
            ptr->typeName = decl.codeName;
            originMap.insert(ptr->getSequenceNumber(), i);
            fieldIndexToNodeMap.insert(i, ptr);
        }break;
        }
    }

    QVector<int> seqNumberTable;
    Tree newTree(builder, seqNumberTable);
    if (newTree.isEmpty()) {
        // this means something goes wrong
        // no matter what, we should at least have a root node
        return false;
    }
    result = ConfigurationData(newTree);

    fieldIndexToNodeIndexMap.clear();
    for (int i = 0; i < seqNumberTable.size(); ++i) {
        int fieldIndex = originMap.value(seqNumberTable.at(i), -2);
        Q_ASSERT(fieldIndex > -2);
        fieldIndexToNodeIndexMap.insert(fieldIndex, i);
    }
    return true;
}

void ConfigurationInputWidget::refreshForm()
{
    if (fieldData.isEmpty())
        return;

    Q_ASSERT(configDecl);

    // step 0: get all data for decision making
    ConfigurationData data;
    QHash<int, int> fieldIndexToNodeIndexMap;
    if (!getConfigurationData(data, fieldIndexToNodeIndexMap))
        return;

    // step 1: decide what to enable / disable
    struct FieldDecision {
        bool valid = false; // true if the decision is valid
        bool enabled = false; // true if the field is enabled, false if disabled
    };
    QVector<FieldDecision> decisionVec(fieldData.size(), FieldDecision());

    for (int i = 0, n = configDecl->getNumFields(); i < n; ++i) {
        auto& decl = configDecl->getField(i);
        // step 1: check if parent is enabled
        int parentIndex = i - decl.indexOffsetFromParent;
        if (parentIndex != -1){
            auto& parentStatus = decisionVec.at(parentIndex);
            if (!(parentStatus.valid && parentStatus.enabled)) {
                // parent disabled
                // skip this one
                continue;
            }
        }
        auto& curStatus = decisionVec[i];
        curStatus.valid = true;

        auto& predVec = decl.predicates;
        if (predVec.isEmpty()) {
            // always enabled
            curStatus.enabled = true;
        } else {
            curStatus.enabled = false;
            Tree::EvaluationContext ctx(data);
            ctx.startNodeIndex = fieldIndexToNodeIndexMap.value(parentIndex, -2);
            Q_ASSERT(ctx.startNodeIndex >= 0);
            for (const auto& subVec : predVec) {
                bool isGood = false;
                for (const auto& pred : subVec) {
                    if (!Tree::evaluatePredicate(pred, isGood, ctx)) {
                        isGood = false;
                        break;
                    }
                }
                if (isGood) {
                    curStatus.enabled = true;
                    break;
                }
            }
        }
    }

    // step 2: actually perform the form update
    for (int i = 0, n = fieldData.size(); i < n; ++i) {
        const auto& decision = decisionVec.at(i);
        if (decision.valid) {
            auto& field = fieldData[i];
            if (field.isEnabled == decision.enabled)
                continue;
            if (decision.enabled) {
                // install it to the form
                if (field.label) {
                    field.parentLayout->addRow(field.label, field.inputWidget);
                    field.label->show();
                    field.inputWidget->show();
                } else {
                    field.parentLayout->addRow(field.inputWidget);
                    field.inputWidget->show();
                }
            } else {
                // remove it from the form
                if (field.label) {
                    field.label->hide();
                    field.parentLayout->removeWidget(field.label);
                }
                field.inputWidget->hide();
                field.parentLayout->removeWidget(field.inputWidget);
            }
            field.isEnabled = decision.enabled;
        }
    }
}

