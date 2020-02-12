#include "src/utils/XMLUtilities.h"

QString StringCache::operator()(QStringRef str) {
    {
        auto iter = hotCache.find(str);
        if (iter != hotCache.end()) {
            return *(*iter).string();
        }
    }
    {
        auto iter = coldCache.find(str);
        if (iter != coldCache.end()) {
            // move it to hot cache
            const QString* ptr = (*iter).string();
            hotCache.insert(*iter);
            coldCache.erase(iter);
            return *ptr;
        }
    }
    // it is in neither cache
    // create the new one
    QString* ptr = new QString;
    ptr->append(str);
    stringTable.push_back(ptr);
    coldCache.insert(QStringRef(ptr));
    return *ptr;
}

StringCache::~StringCache() {
    for (auto ptr : stringTable) {
        delete ptr;
    }
}

bool XMLUtil::readEnum(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& enumElement,
        std::function<bool(QStringRef)> enumReadCB,
        std::initializer_list<QString> possibleValues)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, enumElement);
    }
    if (Q_UNLIKELY(xml.name() != enumElement)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, enumElement);
    }
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
        return XMLError::notHavingCharacter(qWarning(), xml, currentElement, enumElement);
    }
    if (Q_UNLIKELY(!enumReadCB(xml.text()))) {
        return XMLError::invalidEnumString(
            qWarning(), xml, currentElement, enumElement, possibleValues);
    }
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, enumElement);
    }
    return true;
}

bool XMLUtil::readAttribute(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& attributedElement,
        const QString& attributeName,
        std::function<bool(QStringRef)> attributeReadCB,
        std::initializer_list<QString> possibleValues,
        const char* const reason)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto attr = xml.attributes();
    if (Q_UNLIKELY(!attr.hasAttribute(attributeName))) {
        return XMLError::missingAttribute(qWarning(), xml, currentElement, (attributedElement.isEmpty()? xml.name() : QStringRef(&attributedElement)), attributeName);
    }
    auto value = attr.value(attributeName);
    if (Q_UNLIKELY(!attributeReadCB(value))) {
        if (reason) {
            return XMLError::invalidValue(qWarning(), xml, currentElement, attributeName, value, reason);
        } else {
            return XMLError::invalidEnumString(
                qWarning(), xml, currentElement, attributeName, possibleValues);
        }
    }
    return true;
}

bool XMLUtil::readStringList(QXmlStreamReader& xml, const char* const currentElement,
        const QString& stringListElement,
        const QString& listEntryElementName,
        QStringList& list,
        StringCache& strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, stringListElement);
    }
    if (Q_UNLIKELY(xml.name() != stringListElement)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, stringListElement);
    }
    list.clear();
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (Q_UNLIKELY(xml.name() != listEntryElementName)) {
            return XMLError::unexpectedElement(qWarning(), xml, currentElement, listEntryElementName);
        }
        if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
            return XMLError::notHavingCharacter(qWarning(), xml, currentElement, listEntryElementName);
        }
        list.push_back(strCache(xml.text()));
        if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
            return XMLError::missingEndElement(qWarning(), xml, currentElement, listEntryElementName);
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, stringListElement);
    }
    return true;
}

bool XMLUtil::readGeneralList(QXmlStreamReader& xml, const char* const currentElement,
                              const QString& listElement,
                              const QString& listEntryElementName,
                              std::function<bool(QXmlStreamReader&, StringCache&)> readElementCallback,
                              StringCache &strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, listElement);
    }
    if (Q_UNLIKELY(xml.name() != listElement)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, listElement);
    }
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (Q_UNLIKELY(xml.name() != listEntryElementName)) {
            return XMLError::unexpectedElement(qWarning(), xml, currentElement, listEntryElementName);
        }
        if (Q_UNLIKELY(!readElementCallback(xml, strCache))) {
            return XMLError::failOnChild(qWarning(), currentElement, listEntryElementName);
        }
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, listElement);
    }
    return true;
}
