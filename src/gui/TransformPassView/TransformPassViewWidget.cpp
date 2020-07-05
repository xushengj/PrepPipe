#include "TransformPassViewWidget.h"
#include "ui_TransformPassViewWidget.h"

#include <QVBoxLayout>

TransformPassViewWidget::TransformPassViewWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransformPassViewWidget),
    inputDataGroupBoxLayout(new QVBoxLayout),
    outputDataGroupBoxLayout(new QVBoxLayout)
{
    ui->setupUi(this);
    ui->inputDataGroupBox->setLayout(inputDataGroupBoxLayout);
    ui->outputDataGroupBox->setLayout(outputDataGroupBoxLayout);

    ui->stackedWidget->setCurrentWidget(ui->placeholderPage);
}

TransformPassViewWidget::~TransformPassViewWidget()
{
    delete ui;
}

void TransformPassViewWidget::transformRestarted()
{
    qFatal("Not supported yet");
}

void TransformPassViewWidget::installDelegateObject(TransformPassViewDelegateObject* obj) {
    delegateObject = obj;
}

void TransformPassViewWidget::dataReady(EventLogger* e)
{
    Q_ASSERT(e && !events);
    Q_ASSERT(delegateObject);
    events.reset(e->clone());

    delegateObject->updateDataWidgetForNewData();
    inputDataWidget = delegateObject->getInputDataWidget(this);
    outputDataWidget = delegateObject->getOutputDataWidget(this);
    Q_ASSERT(inputDataWidget);
    Q_ASSERT(outputDataWidget);

    inputDataGroupBoxLayout->insertWidget(0, inputDataWidget);
    outputDataGroupBoxLayout->insertWidget(0, outputDataWidget);
    ui->stackedWidget->setCurrentWidget(ui->resultPage);

    buildEventLocationMap();
}

void TransformPassViewWidget::buildEventLocationMap()
{
    inputDataStartPosMap.reset(new EventLocationMapType(delegateObject->getInputDataPositionComparator()));
    outputDataStartPosMap.reset(new EventLocationMapType(delegateObject->getOutputDataPositionComparator()));
    for (int i = 0, n = events->size(); i < n; ++i) {
        const auto& e = events->getEvent(i);
        if (e.locationRemarks.isEmpty())
            continue;

        QVariant inStart;
        QVariant inEnd;
        QVariant outStart;
        QVariant outEnd;
        bool inPosSet = false;
        bool outPosSet = false;
        for (const auto& remark : e.locationRemarks) {
            switch (remark.locationTypeIndex) {
            default: break;
            case static_cast<int>(EventLocationType::InputDataStart): {
                inStart = remark.location;
                inPosSet = true;
            }break;
            case static_cast<int>(EventLocationType::InputDataEnd): {
                inEnd = remark.location;
                Q_ASSERT(inPosSet);
            }break;
            case static_cast<int>(EventLocationType::OutputDataStart): {
                outStart = remark.location;
                outPosSet = true;
            }break;
            case static_cast<int>(EventLocationType::OutputDataEnd): {
                outEnd = remark.location;
                Q_ASSERT(outPosSet);
            }break;
            }
        }

        if (inPosSet) {
            inputDataStartPosMap->insert(std::make_pair(inStart, std::make_pair(inEnd, i)));
        }
        if (outPosSet) {
            outputDataStartPosMap->insert(std::make_pair(outStart, std::make_pair(outEnd, i)));
        }
    }
}

void TransformPassViewWidget::inputDataEventRangeLookupRequested (const QVariant& start, const QVariant& end)
{
    // TODO
}

void TransformPassViewWidget::outputDataEventRangeLookupRequested(const QVariant& start, const QVariant& end)
{
    // TODO
}

void TransformPassViewWidget::inputDataEventSinglePositionLookupRequested (const QVariant& pos)
{
    // TODO
}

void TransformPassViewWidget::outputDataEventSinglePositionLookupRequested(const QVariant& pos)
{
    // TODO
}
