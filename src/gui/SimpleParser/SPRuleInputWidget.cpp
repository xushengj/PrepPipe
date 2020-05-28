#include "SPRuleInputWidget.h"
#include "ui_SPRuleInputWidget.h"

SPRuleInputWidget::SPRuleInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPRuleInputWidget)
{
    ui->setupUi(this);
    ui->parentListWidget->setAcceptGotoRequest(true);
    connect(ui->parentListWidget, &NameListWidget::gotoRequested, this, &SPRuleInputWidget::gotoRuleNodeRequested);
    connect(ui->patternComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SPRuleInputWidget::currentPatternChanged);
    connect(ui->addPatternPushButton, &QPushButton::clicked, this, &SPRuleInputWidget::addPatternRequested);
}

void SPRuleInputWidget::setParentNodeNameCheckCallback(std::function<bool(const QString&)> cb) {
    ui->parentListWidget->setCheckNameCallback(cb);
}

SPRuleInputWidget::~SPRuleInputWidget()
{
    delete ui;
}

void SPRuleInputWidget::nameUpdated(const QString& name)
{
    defaultNodeTypeName = name;
}

void SPRuleInputWidget::setData(const SimpleParser::MatchRuleNode& dataArg)
{
    defaultNodeTypeName = dataArg.name;
    ui->parentListWidget->setData(dataArg.parentNodeNameList);
    for (auto& data : patterns) {
        ui->patternStackedWidget->removeWidget(data.widget);
        delete data.widget;
    }
    patterns.clear();
    ui->patternComboBox->clear();
    for (const auto& p : dataArg.patterns) {
        int index = patterns.size();
        SPRulePatternInputWidget* w = new SPRulePatternInputWidget;
        w->setContentTypeCheckCallback(contentTypeCheckCB);
        w->setNamedBoundaryCheckCallback(namedBoundaryCheckCB);
        w->setData(p);
        PatternData data;
        data.widget = w;
        patterns.push_back(data);
        connect(w, &SPRulePatternInputWidget::dirty, this, &SPRuleInputWidget::dirty);
        connect(w, &SPRulePatternInputWidget::patternTypeNameChanged, this, [=](){
            updatePatternName(w);
        });//updatePatternName
        ui->patternStackedWidget->addWidget(w);
        ui->patternComboBox->addItem(getPatternSummaryName(index, p.typeName));
    }
    if (!patterns.isEmpty()) {
        ui->patternComboBox->setCurrentIndex(0);
    }
}

void SPRuleInputWidget::getData(const QString &name, SimpleParser::MatchRuleNode& dataArg)
{
    dataArg.name = name;
    dataArg.parentNodeNameList = ui->parentListWidget->getData();
    dataArg.patterns.clear();
    int numPatterns = patterns.size();
    dataArg.patterns.resize(numPatterns);
    for (int i = 0; i < numPatterns; ++i) {
        SPRulePatternInputWidget* w = patterns.at(i).widget;
        w->getData(dataArg.patterns[i]);
    }
}

QString SPRuleInputWidget::getPatternSummaryName(int index, const QString& outputTypeName)
{
    return QString("[%1] %2").arg(QString::number(index), outputTypeName);
}

void SPRuleInputWidget::currentPatternChanged(int index)
{
    if (index == -1) {
        ui->patternStackedWidget->setCurrentWidget(ui->placeHolderPage);
    } else {
        Q_ASSERT(index >= 0 && index < patterns.size());
        ui->patternStackedWidget->setCurrentWidget(patterns.at(index).widget);
    }
}

void SPRuleInputWidget::updatePatternName(SPRulePatternInputWidget* widget)
{
    int index = -1;
    for (auto& p : patterns) {
        index += 1;
        if (p.widget != widget)
            continue;

        ui->patternComboBox->setItemText(index, getPatternSummaryName(index, widget->getPatternTypeName()));
        return;
    }
    qFatal("No backing pattern data found");
}

void SPRuleInputWidget::addPatternRequested()
{
    Q_ASSERT(helperPtr);
    SPRulePatternQuickInputDialog* dialog = new SPRulePatternQuickInputDialog(*helperPtr, defaultNodeTypeName, this);
    if (dialog->exec() == QDialog::Accepted) {
        // TODO
    }
    dialog->deleteLater();
}
