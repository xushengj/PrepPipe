#include "TransformPassViewWidget.h"
#include "ui_TransformPassViewWidget.h"

#include <QVBoxLayout>
#include <QRandomGenerator>

#include <iterator>
#include <algorithm>

#include "src/lib/TaskObject.h"

TransformPassViewWidget::TransformPassViewWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransformPassViewWidget)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentWidget(ui->placeholderPage);

    eventListModel = new TransformEventListModel(this);
    ui->topEventFilterCheckBox->setChecked(eventListModel->isHidingSecondaryEvent());
    connect(ui->topEventFilterCheckBox, &QCheckBox::toggled, eventListModel, &TransformEventListModel::toggleHideSecondaryEvents);
    ui->eventListView->setModel(eventListModel);

    updateEventDetails(-1); // initialize the text
    connect(ui->eventListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TransformPassViewWidget::handleEventListSelectionChange);
    connect(ui->eventClickableTreeWidget, &QTreeWidget::itemDoubleClicked, this, &TransformPassViewWidget::handleEventClickableItemDoubleClicked);
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
    delegateObject.reset(obj);
}

void TransformPassViewWidget::dataReady(EventLogger* e)
{
    Q_ASSERT(e && !events);
    Q_ASSERT(delegateObject);
    Q_ASSERT(e->isPassSucceeded() || e->isPassFailed()); // not "unknown" exit condition
    events.reset(e->clone());

    delegateObject->updateDataWidgetForNewData();
    int numObjs = delegateObject->getNumDataObjects();
    for (int i = 0; i < numObjs; ++i) {
        const auto& info = delegateObject->getDataInfo(i);
        QWidget* w = delegateObject->getDataWidget(this, i);
        Q_ASSERT(w != nullptr);
        if (info.isOutputInsteadOfInput) {
            ui->outputDataWidgets->addWidget(TaskObject::getOutputDisplayName(info.name), w);
        } else {
            ui->inputDataWidgets->addWidget(TaskObject::getInputDisplayName(info.name), w);
        }
    }
    eventListModel->setLogger(events.get());
    ui->stackedWidget->setCurrentWidget(ui->resultPage);

    buildEventLocationMap();

    // build eventIncomingReferences
    eventIncomingReferences.clear();
    eventIncomingReferences.resize(events->size());
    for (int i = 0, n = events->size(); i < n; ++i) {
        const Event& e = events->getEvent(i);
        for (const auto& ref : e.references) {
            int eventIndex = ref.eventIndex;
            Q_ASSERT(eventIndex >= 0 && eventIndex < i);
            eventIncomingReferences[eventIndex].push_back(std::make_pair(i, ref.referenceTypeIndex));
        }
    }

    eventRecolorRequested();
    if (events->isPassFailed()) {
        // set event focus to the failed event
        setFocusToEvent(events->getFatalEventIndex());
    }
}

TransformPassViewWidget::EventLocationExpansionData TransformPassViewWidget::extractLocationsFromEvent(const Event& e)
{
    TransformPassViewWidget::EventLocationExpansionData result;
    /*
    for (const auto& remark : e.locationRemarks) {
        switch (remark.locationTypeIndex) {
        default: break;
        case static_cast<int>(EventLocationType::InputDataStart): {
            result.inputDataStart = remark.location;
            result.inputPosSet = true;
        }break;
        case static_cast<int>(EventLocationType::InputDataEnd): {
            result.inputDataEnd = remark.location;
            Q_ASSERT(result.inputPosSet);
        }break;
        case static_cast<int>(EventLocationType::OutputDataStart): {
            result.outputDataStart = remark.location;
            result.outputPosSet = true;
        }break;
        case static_cast<int>(EventLocationType::OutputDataEnd): {
            result.outputDataEnd = remark.location;
            Q_ASSERT(result.outputPosSet);
        }break;
        }
    }
    */
    return result;
}

void TransformPassViewWidget::buildEventLocationMap()
{
    /*
    inputDataStartPosMap.reset(new EventLocationMapType(delegateObject->getInputDataPositionComparator()));
    outputDataStartPosMap.reset(new EventLocationMapType(delegateObject->getOutputDataPositionComparator()));
    for (int i = 0, n = events->size(); i < n; ++i) {
        const auto& e = events->getEvent(i);
        if (e.locationRemarks.isEmpty())
            continue;

        EventLocationExpansionData expansion = extractLocationsFromEvent(e);

        if (expansion.inputPosSet) {
            inputDataStartPosMap->insert(std::make_pair(expansion.inputDataStart, std::make_pair(expansion.inputDataEnd, i)));
        }
        if (expansion.outputPosSet) {
            outputDataStartPosMap->insert(std::make_pair(expansion.outputDataStart, std::make_pair(expansion.outputDataEnd, i)));
        }
    }
    */
}

void TransformPassViewWidget::setFocusToEvent(int eventIndex)
{
    eventFocusTy = EventFocusType::Event;
    eventFocusLocation = eventIndex;
    QModelIndex modelIndex = eventListModel->getModelIndexFromEventIndex(eventIndex);
    if (modelIndex.isValid()) {
        QItemSelectionModel* selectionModel = ui->eventListView->selectionModel();
        selectionModel->select(modelIndex, QItemSelectionModel::SelectCurrent);
    }
    updateEventDetails(eventIndex);

    requestRefreshingHighlighting();
}

void TransformPassViewWidget::unsetEventFocus()
{
    eventFocusTy = EventFocusType::NoFocus;
    eventFocusLocation = QVariant();
    requestRefreshingHighlighting();
}

void TransformPassViewWidget::requestRefreshingHighlighting()
{
    if (isRefreshRequested) {
        return;
    }

    isRefreshRequested = true;
    QMetaObject::invokeMethod(this, [this](){refreshHighlighting();}, Qt::QueuedConnection);
}

void TransformPassViewWidget::refreshHighlighting()
{
    isRefreshRequested = false;
//#error TODO
    // we need to do the following things:
    // 1. set up the color for events.
    //    if no event is being focused, color all actively colored events
    //    otherwise, find all related events that should be colored and color them
    // 2. populate events
    //    if no event is being focused: the event list should be cleared
    //    if we are searching for events near a location, the event list should be the search results
    // 3. move the view if needed
    //    if we are highlighting an event, make sure it is visible in input/output viewer
    delegateObject->resetDataWidgetEventColoring(-1);
    switch (eventFocusTy) {
    case EventFocusType::NoFocus: {
        // TODO
    }break;
    case EventFocusType::Event: {
        // find whether we have location information for the event
        // if yes, then highlight it in corresponding widget
    }break;
    case EventFocusType::Input: {
        // TODO
    }break;
    case EventFocusType::Output: {
        // TODO
    }break;
    }
}

void TransformPassViewWidget::updateEventDetails(int eventIndex)
{
    if (lastDetailedEventIndex == eventIndex)
        return;

    lastDetailedEventIndex = eventIndex;
    ui->eventClickableTreeWidget->clear();
    currentEventLocations.clear();
    currentEventCrossReferences.clear();
    if (eventIndex < 0) {
        ui->eventNameLabel->setText(tr("(No event selection)"));
        ui->eventDocumentLabel->setText(tr("Select an event to view the details"));
    } else {
        Q_ASSERT(eventIndex < events->size());
        const Event& e = events->getEvent(eventIndex);
        const auto* interp = events->getInterpreter();
        QString nameLabel = TransformEventListModel::getEventNameLabel(events.get(), interp, eventIndex);
        ui->eventNameLabel->setText(nameLabel);
        ui->eventDocumentLabel->setText(interp->getDetailString(events.get(), eventIndex));

        if (!e.locationRemarks.isEmpty()) {
            QTreeWidgetItem* locationHead = new QTreeWidgetItem;
            locationHead->setText(0, tr("Locations"));
            ui->eventClickableTreeWidget->addTopLevelItem(locationHead);
            locationHead->setExpanded(true);
            for (indextype i = 0, n = e.locationRemarks.size(); i < n; ++i) {
                const auto& loc = e.locationRemarks.at(i);
                QString locType = interp->getLocationTypeTitle(events.get(), eventIndex, e.eventTypeIndex, i);
                int objectID = loc.locationContextIndex;
                QString locStr = delegateObject->getLocationDescriptionString(objectID, loc.location);
                QString locationStr;
                if (locType.isEmpty()) {
                    locationStr = locStr;
                } else {
                    locationStr = locType;
                    locationStr.append(QStringLiteral(": "));
                    locationStr.append(locStr);
                }
                QTreeWidgetItem* locItem = new QTreeWidgetItem;
                locItem->setText(0, locationStr);
                currentEventLocations.insert(locItem, std::make_pair(objectID, loc.location));
                locationHead->addChild(locItem);
            }
        }
        // TODO: add support of event
        /*
        QHash<int, std::pair<QVariant, QVariant>> locations;
        for (const auto& remark : e.locationRemarks) {
            Q_ASSERT(remark.locationTypeIndex >= 0);
            int base = remark.locationTypeIndex / 2;
            if (remark.locationTypeIndex % 2 == 1) {
                locations[base].second = remark.location;
            } else {
                locations[base].first = remark.location;
            }
        }
        */
        // analyze cross reference
        // add "what does this event references"
        if (!e.references.isEmpty()) {
            QTreeWidgetItem* outgoingRefHead = new QTreeWidgetItem;
            outgoingRefHead->setText(0, tr("References"));
            ui->eventClickableTreeWidget->addTopLevelItem(outgoingRefHead);
            outgoingRefHead->setExpanded(true);

            // here we try to group events under the same reference type
            int eventGroupStart = 0;
            auto commitGroup = [&](int eventGroupEndIndex) -> void {
                const auto& refHead = e.references.at(eventGroupStart);
                QString referenceTypeName = interp->getReferenceTypeTitle(events.get(), eventIndex, e.eventTypeIndex, refHead.referenceTypeIndex);
                if (eventGroupStart == eventGroupEndIndex) {
                    // we only have one reference in this group
                    QTreeWidgetItem* refItem = new QTreeWidgetItem;
                    QString labelString = referenceTypeName;
                    labelString.append(": ");
                    labelString.append(TransformEventListModel::getEventNameLabel(events.get(), interp, refHead.eventIndex));
                    refItem->setText(0, labelString);
                    currentEventCrossReferences.insert(refItem, refHead.eventIndex);
                    outgoingRefHead->addChild(refItem);
                } else {
                    // we have multiple one
                    QTreeWidgetItem* refGroup = new QTreeWidgetItem;
                    refGroup->setText(0, referenceTypeName);
                    outgoingRefHead->addChild(refGroup);
                    refGroup->setExpanded(true);
                    for (int i = eventGroupStart; i <= eventGroupEndIndex; ++i) {
                        const auto& ref = e.references.at(i);
                        QTreeWidgetItem* refItem = new QTreeWidgetItem;
                        refItem->setText(0, TransformEventListModel::getEventNameLabel(events.get(), interp, ref.eventIndex));
                        currentEventCrossReferences.insert(refItem, ref.eventIndex);
                        refGroup->addChild(refItem);
                    }
                }
                eventGroupStart = eventGroupEndIndex + 1;
            };
            for (int i = 1, n = e.references.size(); i < n; ++i) {
                const auto& ref = e.references.at(i);
                const auto& refHead = e.references.at(eventGroupStart);
                if (ref.referenceTypeIndex != refHead.referenceTypeIndex) {
                    commitGroup(i - 1);
                }
            }
            commitGroup(e.references.size() - 1);
        }
        const auto& incomingRefVec = eventIncomingReferences.at(eventIndex);
        if (!incomingRefVec.isEmpty()) {
            QTreeWidgetItem* incomingRefHead = new QTreeWidgetItem;
            incomingRefHead->setText(0, tr("Referenced by"));
            ui->eventClickableTreeWidget->addTopLevelItem(incomingRefHead);
            incomingRefHead->setExpanded(true);
            for (const auto& ref : incomingRefVec) {
                int eventIndex = ref.first;
                int referenceTypeIndex = ref.second;
                const Event& srcEvent = events->getEvent(eventIndex);
                QTreeWidgetItem* refItem = new QTreeWidgetItem;
                QString labelName = TransformEventListModel::getEventNameLabel(events.get(), interp, eventIndex);
                labelName.append(" (");
                labelName.append(interp->getReferenceTypeTitle(events.get(), eventIndex, srcEvent.eventTypeIndex, referenceTypeIndex));
                labelName.append(")");
                refItem->setText(0, labelName);
                currentEventCrossReferences.insert(refItem, eventIndex);
                incomingRefHead->addChild(refItem);
            }
        }
    }
}

void TransformPassViewWidget::handleEventListSelectionChange(const QModelIndex& current, const QModelIndex& prev)
{
    Q_UNUSED(prev)

    if (current.isValid()) {
        int eventIndex = eventListModel->getEventIndexFromListIndex(current.row());
        updateEventDetails(eventIndex);
    }
}

void TransformPassViewWidget::handleEventClickableItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    // TODO add highlighting for locations
    // for now only event references are implemented
    int eventIndex = currentEventCrossReferences.value(item, -1);
    if (eventIndex >= 0) {
        setFocusToEvent(eventIndex);
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

void TransformPassViewWidget::eventRecolorRequested()
{
    // TODO
    activelyColoredEventColors.clear();
    int numColors = events->getNumActivelyColoredEvents();
    activelyColoredEventColors.reserve(numColors);

    // compute luminance from rgb color (ITU BT.709)
    // reference: https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
    // Y = 0.2126 R + 0.7152 G + 0.0722 B
    auto getLuminance = [](int r, int g, int b) -> double {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    };

    // a full-fledged algorithm should take into account the distance between events
    // and avoid assigning similar colors to adjacent events
    // however, for now we will just randomly create colors

    // we always use the background color to represent events.
    for (int i = 0; i < numColors; ++i) {
        int r = 0, g = 0, b = 0;
        do {
            r = QRandomGenerator::global()->generate() % 256;
            g = QRandomGenerator::global()->generate() % 256;
            b = QRandomGenerator::global()->generate() % 256;
            // make sure the generated color has contrast ratio if around 12:1 w.r.t black text,
            // which turns to ~0.55 luminance
            // if the generated color is not too dark, we can scale the value up
            // otherwise we have to regenerate them
            double curLuminance = getLuminance(r, g, b);
            if (curLuminance < 0.1) {
                continue;
            }
            double scaleFactor = 0.55 / curLuminance;
            r = scaleFactor * r;
            g = scaleFactor * g;
            b = scaleFactor * b;
            break;
        }while(0);

        activelyColoredEventColors.push_back(QColor(r, g, b));
    }

    requestRefreshingHighlighting();
}

// -----------------------------------------------------------------------------

TransformEventListModel::TransformEventListModel(QObject* parent)
    : QAbstractListModel(parent)
{

}

void TransformEventListModel::setLogger(EventLogger* loggerArg)
{
    beginResetModel();
    logger = loggerArg;
    recomputeFilteredEvents();
    endResetModel();
}

void TransformEventListModel::setFilter(std::function<bool(EventLogger*,int)> filterCB)
{
    beginResetModel();
    recomputeFilteredEvents(filterCB);
    endResetModel();
}

void TransformEventListModel::toggleHideSecondaryEvents(bool isHide)
{
    bool isReset = false;
    if (isHideSecondaryEvents != isHide) {
        isReset = true;
    }
    if (isReset) {
        beginResetModel();
    }
    isHideSecondaryEvents = isHide;
    if (isReset) {
        endResetModel();
    }
}

QModelIndex TransformEventListModel::getModelIndexFromEventIndex(int eventIndex) const
{
    if (isHideSecondaryEvents) {
        auto iter = std::lower_bound(topLevelFinalEventVec.begin(), topLevelFinalEventVec.end(), eventIndex);
        if (iter == topLevelFinalEventVec.end() || (*iter != eventIndex)) {
            return QModelIndex();
        }

        int index = std::distance(topLevelFinalEventVec.begin(), iter);
        return createIndex(index, 0);
    }

    if (filteredEventVec.isEmpty()) {
        if (topLevelFinalEventVec.isEmpty()) {
            // no event is in the list
            return QModelIndex();
        }

        // all events are in the list, and the row is just the event index
        return createIndex(eventIndex, 0);
    }

    // if this is the case then the event must be in filteredEventVec
    auto iter = std::lower_bound(filteredEventVec.begin(), filteredEventVec.end(), eventIndex);
    if (iter == filteredEventVec.end() || (*iter != eventIndex)) {
        return QModelIndex();
    }

    int index = std::distance(filteredEventVec.begin(), iter);
    return createIndex(index, 0);
}

void TransformEventListModel::recomputeFilteredEvents(std::function<bool(EventLogger*,int)> filterCB)
{
    filteredEventVec.clear();
    topLevelFinalEventVec.clear();
    if (filterCB) {
        // filter is valid
        for (int i = 0, n = logger->size(); i < n; ++i) {
            if (filterCB(logger, i)) {
                filteredEventVec.push_back(i);
            }
        }
        if (filteredEventVec.isEmpty()) {
            // no event is selected
            return;
        }
    }
    // filteredEventVec is now ready
    // populate topLevelFinalEventVec
    int num = logger->size();
    if (num == 0) {
        return;
    }

    std::vector<bool> scetchpad(num, false);

    // helper function
    std::function<void(int, bool)> floodFill = [&](int index, bool isReferenced) {
        if (isReferenced) {
            scetchpad.at(index) = true;
        }
        auto& event = logger->getEvent(index);
        for (const auto& ref : event.references) {
            int refIndex = ref.eventIndex;
            Q_ASSERT(refIndex < index);
            floodFill(refIndex, true);
        }
    };

    const EventInterpreter* interp = logger->getInterpreter();
    auto isPrimaryEvent = [&](int eventIndex) -> bool {
        const auto& e = logger->getEvent(eventIndex);
        switch (interp->getEventImportance(e.eventTypeIndex)) {
        case EventInterpreter::EventImportance::AlwaysPrimary:   return true;
        case EventInterpreter::EventImportance::AlwaysSecondary: return false;
        case EventInterpreter::EventImportance::Auto: return !scetchpad.at(eventIndex);
        default: qFatal("Unhandled event importance"); return false;
        }
    };
    if (filteredEventVec.isEmpty()) {
        for (int i = num-1; i >= 0; --i) {
            if (!scetchpad.at(i)) {
                floodFill(i, false);
            }
        }

        for (int i = 0; i < num; ++i) {
            if (isPrimaryEvent(i)) {
                topLevelFinalEventVec.push_back(i);
            }
        }
    } else {
        for (int i = filteredEventVec.size()-1; i >= 0; --i) {
            int eventIndex = filteredEventVec.at(i);
            if (!scetchpad.at(eventIndex)) {
                floodFill(eventIndex, false);
            }
        }
        for (int i = filteredEventVec.size()-1; i >= 0; --i) {
            int eventIndex = filteredEventVec.at(i);
            if (isPrimaryEvent(eventIndex)) {
                topLevelFinalEventVec.push_back(eventIndex);
            }
        }
    }
}

int TransformEventListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return getNumEventInList();
}

int TransformEventListModel::getNumEventInList() const
{
    if (isHideSecondaryEvents) {
        return topLevelFinalEventVec.size();
    }
    if (filteredEventVec.isEmpty()) {
        // either a filter is applied and no result is available, or no filter is applied
        if (topLevelFinalEventVec.isEmpty()) {
            return 0;
        }
        return logger->size();
    } else {
        return filteredEventVec.size();
    }
}

int TransformEventListModel::getEventIndexFromListIndex(int listIndex) const
{
    if (isHideSecondaryEvents) {
        return topLevelFinalEventVec.at(listIndex);
    }

    if (filteredEventVec.isEmpty()) {
        // either a filter is applied and no result is available, or no filter is applied
        // here we assume the second case, because in the first case we won't have this function being called
        return listIndex;
    }

    return filteredEventVec.at(listIndex);
}

QVariant TransformEventListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= getNumEventInList())
        return QVariant();


    switch (index.column()) {
    default: return QVariant();
    case 0: {
        int eventIndex = getEventIndexFromListIndex(index.row());
        const auto* interp = logger->getInterpreter();
        switch (role) {
        case Qt::DisplayRole: return getEventNameLabel(logger, interp, eventIndex);
        case Qt::ToolTipRole: return interp->getDetailString(logger, eventIndex);
        }
    }break;
    }

    return QVariant();
}

QString TransformEventListModel::getEventNameLabel(const EventLogger* logger, const EventInterpreter* interp, int eventIndex)
{
    QString label;
    label.append('#');
    label.append(QString::number(eventIndex));
    label.append(' ');
    label.append(interp->getEventTitle(logger, eventIndex, logger->getEvent(eventIndex).eventTypeIndex));
    return label;
}
