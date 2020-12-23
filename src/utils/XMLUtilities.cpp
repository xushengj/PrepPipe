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

bool XMLUtil::readString(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& stringElement,
        QString& str,
        StringCache& strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, stringElement);
    }
    if (Q_UNLIKELY(xml.name() != stringElement)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, stringElement);
    }
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
        // also accept when string is empty
        if (Q_LIKELY(xml.tokenType() == QXmlStreamReader::EndElement)) {
            str.clear();
            return true;
        }
        return XMLError::notHavingCharacter(qWarning(), xml, currentElement, stringElement);
    }
    str = strCache(xml.text());
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, stringElement);
    }
    return true;
}

bool XMLUtil::readRemainingString(
        QXmlStreamReader& xml, const char* const currentElement,
        QString& str, StringCache &strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    QStringRef stringElement = xml.name();
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
        // also accept when string is empty
        if (Q_LIKELY(xml.tokenType() == QXmlStreamReader::EndElement)) {
            str.clear();
            return true;
        }
        return XMLError::notHavingCharacter(qWarning(), xml, currentElement, stringElement.toString());
    }
    str = strCache(xml.text());
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, stringElement.toString());
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

bool XMLUtil::readOptionalAttribute(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& attributedElement,
        const QString& attributeName,
        std::function<bool(QStringRef)> attributeReadCB,
        std::initializer_list<QString> possibleValues,
        const char* const reason)
{
    Q_UNUSED(attributedElement)
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto attr = xml.attributes();
    if (!attr.hasAttribute(attributeName)) {
        // it is not a problem if the optional attribute is missing
        return true;
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

bool XMLUtil::readStringAttribute(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& attributedElement,
        const QString& attributeName,
        QString& string,
        StringCache& strCache)
{
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
    auto attr = xml.attributes();
    if (Q_UNLIKELY(!attr.hasAttribute(attributeName))) {
        return XMLError::missingAttribute(qWarning(), xml, currentElement, (attributedElement.isEmpty()? xml.name() : QStringRef(&attributedElement)), attributeName);
    }
    auto value = attr.value(attributeName);
    string = strCache(value);
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
            if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
                return XMLError::notHavingCharacter(qWarning(), xml, currentElement, listEntryElementName);
            } else {
                // empty strinng
                list.push_back(QString());
            }
        } else {
            list.push_back(strCache(xml.text()));
            if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
                return XMLError::missingEndElement(qWarning(), xml, currentElement, listEntryElementName);
            }
        }
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, stringListElement);
    }
    return true;
}

void XMLUtil::writeStringList(QXmlStreamWriter& xml, const QStringList& list, const QString& stringListElement, const QString& listEntryElementName, bool sort)
{
    xml.writeStartElement(stringListElement);
    if (sort) {
        QStringList tmp = list;
        NameSorting::sortNameList(tmp);
        for (const auto& str : tmp) {
            xml.writeTextElement(listEntryElementName, str);
        }
    } else {
        for (const auto& str : list) {
            xml.writeTextElement(listEntryElementName, str);
        }
    }
    xml.writeEndElement();
}

void XMLUtil::writeStringListHash(
        QXmlStreamWriter& xml,
        const QHash<QString, QStringList>& hash,
        const QString& hashName,
        const QString& hashEntryName,
        const QString& hashKeyAttributeName,
        const QString& stringElementName,
        bool sortString)
{
    xml.writeStartElement(hashName);
    QStringList keys = hash.keys();
    NameSorting::sortNameList(keys);
    for (const auto& key : keys) {
        auto iter = hash.find(key);
        xml.writeStartElement(hashEntryName);
        xml.writeAttribute(hashKeyAttributeName, key);
        QStringList list = iter.value();
        if (sortString) {
            QStringList tmp = list;
            NameSorting::sortNameList(tmp);
            for (const auto& str : tmp) {
                xml.writeTextElement(stringElementName, str);
            }
        } else {
            for (const auto& str : list) {
                xml.writeTextElement(stringElementName, str);
            }
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

bool XMLUtil::readStringListHash(QXmlStreamReader& xml,
        const char* const currentElement,
        const QString& hashName,
        const QString& hashEntryName,
        const QString& hashKeyName,
        const QString& stringElementName,
        QHash<QString, QStringList>& hash,
        StringCache &strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, hashName);
    }
    if (Q_UNLIKELY(xml.name() != hashName)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, hashName);
    }
    hash.clear();
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (Q_UNLIKELY(xml.name() != hashEntryName)) {
            return XMLError::unexpectedElement(qWarning(), xml, currentElement, hashEntryName);
        }
        QString key;
        if (Q_UNLIKELY(!readStringAttribute(xml, currentElement, hashEntryName, hashKeyName, key, strCache))) {
            return XMLError::failOnChild(qWarning(), currentElement, hashEntryName);
        }
        QStringList list;
        {
            while (xml.readNextStartElement()) {
                Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
                if (Q_UNLIKELY(xml.name() != stringElementName)) {
                    return XMLError::unexpectedElement(qWarning(), xml, currentElement, stringElementName);
                }
                if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
                    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
                        return XMLError::notHavingCharacter(qWarning(), xml, currentElement, stringElementName);
                    } else {
                        // empty strinng
                        list.push_back(QString());
                    }
                } else {
                    list.push_back(strCache(xml.text()));
                    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
                        return XMLError::missingEndElement(qWarning(), xml, currentElement, stringElementName);
                    }
                }
            }
            if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
                return XMLError::missingEndElement(qWarning(), xml, currentElement, stringElementName);
            }
        }
        hash.insert(key, list);
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, hashName);
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

bool XMLUtil::readElementText(QXmlStreamReader& xml, const char* const currentElement, const QString& textElement, QString &str, StringCache &strCache)
{
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::Characters)) {
        return XMLError::notHavingCharacter(qWarning(), xml, currentElement, textElement);
    }
    str = strCache(xml.text());
    if (Q_UNLIKELY(xml.readNext() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, textElement);
    }
    return true;
}

void XMLUtil::writeFlagElement(QXmlStreamWriter& xml, std::initializer_list<std::pair<bool, QString>> flags, const QString& flagElementName)
{
    xml.writeStartElement(flagElementName);
    for (auto& f : flags) {
        xml.writeAttribute(f.second, (f.first? QStringLiteral("Yes"): QStringLiteral("No")));
    }
    xml.writeEndElement();
}

bool XMLUtil::readFlagElement(QXmlStreamReader& xml, const char* const currentElement, std::initializer_list<std::pair<bool&, QString>> flags, const QString& flagElementName, StringCache &strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, flagElementName);
    }
    if (Q_UNLIKELY(xml.name() != flagElementName)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, flagElementName);
    }
    auto attrs = xml.attributes();
    for (auto& f : flags) {
        if (attrs.hasAttribute(f.second)) {
            QStringRef text = attrs.value(f.second);
            if (text == QStringLiteral("Yes")) {
                f.first = true;
            } else if (text == QStringLiteral("No")) {
                f.first = false;
            } else {
                return XMLError::invalidValue(qWarning(), xml, currentElement, f.second, text, "value should be either \"Yes\" or \"No\"");
            }
        }
    }
    xml.skipCurrentElement();
    return true;
}
