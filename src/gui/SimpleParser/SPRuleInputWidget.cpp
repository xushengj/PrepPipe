#include "SPRuleInputWidget.h"
#include "ui_SPRuleInputWidget.h"
#include "src/utils/EventLoopHelper.h"

#include <QMessageBox>

SPRuleInputWidget::SPRuleInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPRuleInputWidget)
{
    ui->setupUi(this);
    ui->parentListWidget->setAcceptGotoRequest(true);
    //connect(ui->parentListWidget, &NameListWidget::gotoRequested, this, &SPRuleInputWidget::gotoRuleNodeRequested);
    connect(ui->patternComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SPRuleInputWidget::currentPatternChanged);
    connect(ui->addPatternPushButton, &QPushButton::clicked, this, &SPRuleInputWidget::addPatternRequested);
    connect(ui->deletePatternPushButton, &QPushButton::clicked, this, &SPRuleInputWidget::deleteCurrentPatternRequested);
    updateButtonState();
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

void SPRuleInputWidget::otherDataUpdated()
{
    for (auto& d : patterns) {
        d.widget->otherDataUpdated();
    }
}

void SPRuleInputWidget::setData(const SimpleParser::MatchRuleNode& dataArg)
{
    defaultNodeTypeName = dataArg.name;
    for (auto& data : patterns) {
        ui->patternStackedWidget->removeWidget(data.widget);
        delete data.widget;
    }
    patterns.clear();
    ui->patternComboBox->clear();
    for (const auto& p : dataArg.patterns) {
        addPattern(p);
    }
    if (!patterns.isEmpty()) {
        ui->patternComboBox->setCurrentIndex(0);
    }
    updateButtonState();
}

void SPRuleInputWidget::addPattern(const SimpleParser::Pattern& patternData)
{
    int index = patterns.size();
    SPRulePatternInputWidget* w = new SPRulePatternInputWidget;
    w->setContentTypeCheckCallback(contentTypeCheckCB);
    w->setNamedBoundaryCheckCallback(namedBoundaryCheckCB);
    w->setData(patternData);
    PatternData data;
    data.widget = w;
    patterns.push_back(data);
    connect(w, &SPRulePatternInputWidget::dirty, this, &SPRuleInputWidget::dirty);
    connect(w, &SPRulePatternInputWidget::patternTypeNameChanged, this, [=](){
        updatePatternName(w);
    });//updatePatternName
    ui->patternStackedWidget->addWidget(w);
    ui->patternComboBox->addItem(getPatternSummaryName(index, patternData.typeName));
}

void SPRuleInputWidget::updateButtonState()
{
    ui->deletePatternPushButton->setEnabled(!patterns.isEmpty());
}

void SPRuleInputWidget::getData(const QString &name, const HierarchicalElementTreeControllerObject::GraphData *graph, SimpleParser::MatchRuleNode& dataArg)
{
    dataArg.name = name;
    dataArg.childNodeNameList = graph->getChildElementList(name);
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
    if (EventLoopHelper::execDialog(dialog) == QDialog::Accepted) {
        SimpleParser::Pattern data;
        dialog->getData(data);
        if (!data.pattern.isEmpty()) {
            int cnt = ui->patternComboBox->count();
            addPattern(data);
            ui->patternComboBox->setCurrentIndex(cnt);
            updateButtonState();
            emit dirty();
        }
    }
    dialog->deleteLater();
}

void SPRuleInputWidget::deleteCurrentPatternRequested()
{
    Q_ASSERT(!patterns.isEmpty());
    Q_ASSERT(ui->patternComboBox->count() > 0);
    int index = ui->patternComboBox->currentIndex();

    // should be impossible, but just make sure
    if (index < 0)
        return;

    if (QMessageBox::question(
                this,
                tr("Delete pattern confirmation"),
                tr("Delete current pattern \"%1\"?").arg(
                    ui->patternComboBox->currentText()
                ),
                QMessageBox::Yes | QMessageBox::Cancel
        ) != QMessageBox::Yes) {
        return;
    }


    {
        auto& data = patterns.at(index);
        ui->patternStackedWidget->removeWidget(data.widget);
        delete data.widget;
    }
    patterns.removeAt(index);

    // refresh all pattern display name
    ui->patternComboBox->clear();
    for (int i = 0, n = patterns.size(); i < n; ++i) {
        ui->patternComboBox->addItem(getPatternSummaryName(i, patterns.at(i).widget->getPatternTypeName()));
    }

    if (!patterns.isEmpty()) {
        ui->patternComboBox->setCurrentIndex((index < patterns.size())? index : (patterns.size()-1));
    }
    emit dirty();
}
