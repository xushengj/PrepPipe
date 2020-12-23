#ifndef STRINGCACHE_H
#define STRINGCACHE_H

#include <QString>
#include <QStringRef>
#include <QHash>
#include <QXmlStreamReader>
#include <QDebug>

#include <initializer_list>

#include "src/utils/NameSorting.h"

/**
 * @brief The StringCache class reduces duplicated string allocation
 *
 * When parsing XML, we may have a lot of identical data strings.
 * They can add up quickly to consume a lot of memory.
 *
 * This class intends to make use of Qt's implicit sharing. When a new
 * QString is needed, this cache checks whether the string has already
 * appeared before; if yes, then the same string is returned. This
 * ensures that only one copy of the same string will be allocated.
 */
class StringCache {
public:
    QString operator()(QStringRef str);
    StringCache() = default;
    StringCache(const StringCache&) = delete; // prevent unintended copy; this class should always be passed by reference
    StringCache(StringCache&&) = default;
    ~StringCache();
private:
    QSet<QStringRef> hotCache;
    QSet<QStringRef> coldCache;
    QList<QString*> stringTable;
};

namespace XMLUtil {

/**
 * @brief sortNameList performs locale aware sorting on thelist
 * @param list the list to sort
 */
void sortNameList(QStringList& list);

/**
 * @brief readEnum read a enum in form of <enumElement>enum</enumElement>
 * @param xml the xml must be before the StartElement of enumElement
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param enumElement the expected xml element name of enum
 * @param enumReadCB the callback for reading enum; return true on success, false otherwise
 * @param possibleValues the list of legal enum values (for logging purpose only)
 * @return true on success, false otherwise
 */
bool readEnum(QXmlStreamReader& xml, const char* const currentElement,
              const QString& enumElement,
              std::function<bool(QStringRef)> enumReadCB,
              std::initializer_list<QString> possibleValues);

/**
 * @brief readString read a string element in form of <stringElement>str</stringElement>
 * @param xml the xml must be before the StartElement of enumElement
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param stringElement the expected xml element name of the string
 * @param str lvalue reference of string to read
 * @param strCache the string cache
 * @return true on success, false otherwise
 */
bool readString(QXmlStreamReader& xml, const char* const currentElement,
                const QString& stringElement,
                QString& str, StringCache &strCache);

/**
 * @brief readRemainingString read a string element in form of <stringElement>str</stringElement>
 *
 * WARNING: this function will eat the EndElement token
 *
 * @param xml the xml must be currently AT the StartElement of enumElement, not before
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param str lvalue reference of string to read
 * @param strCache the string cache
 * @return true on success, false otherwise
 */
bool readRemainingString(
        QXmlStreamReader& xml, const char* const currentElement,
        QString& str, StringCache &strCache);

/**
 * @brief readAttribute read an attribute when current xml token is StartElement
 * @param xml the xml reader whose current token is StartElement
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param attributedElement the name of element where the attribute is read from; feed in an empty string for using xml.name() (for logging purpose only)
 * @param attributeName the name of attribute to read
 * @param attributeReadCB the callback for reading attribute; return true on success, false otherwise
 * @param possibleValues if the attribute is an enum, this is the list of legal enum values (for logging purpose only)
 * @param reason if the attribute is not an enum, this points to a string for logging why the attribute value is rejected
 * @return true on success, false otherwise
 */
bool readAttribute(QXmlStreamReader& xml, const char* const currentElement,
                   const QString& attributedElement,
                   const QString& attributeName,
                   std::function<bool(QStringRef)> attributeReadCB,
                   std::initializer_list<QString> possibleValues,
                   const char * const reason);

// WARNING: remember to set initial values of the attribute before calling the read function
bool readOptionalAttribute(QXmlStreamReader& xml, const char* const currentElement,
                           const QString& attributedElement,
                           const QString& attributeName,
                           std::function<bool(QStringRef)> attributeReadCB,
                           std::initializer_list<QString> possibleValues,
                           const char * const reason);

inline bool readOptionalIntAttribute(QXmlStreamReader& xml, const char* const currentElement, const QString& attributedElement, const QString& attributeName, int& intVal){
    return readOptionalAttribute(xml, currentElement, attributedElement, attributeName,
        [&](QStringRef str)->bool{
            bool isGood = false;
            intVal = str.toInt(&isGood);
            return isGood;
        }, {}, "the value is not an integer");
}

bool readStringAttribute(QXmlStreamReader& xml, const char* const currentElement,
                         const QString& attributedElement,
                         const QString& attributeName,
                         QString& string,
                         StringCache &strCache);

/**
 * @brief readStringList read a string list
 *
 * The string list is expected to be in the following form:
 * <stringListElement>
 *   <listEntryElementName>...</listEntryElementName>
 *   ...
 *   <listEntryElementName>...</listEntryElementName>
 * </stringListElement>
 *
 * @param xml the xml must be before the StartElement of stringListElement
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param stringListElement the xml element name for string list
 * @param listEntryElementName the xml element name for string entry
 * @param list the reference to result list
 * @param strCache the string cache
 * @return true on success, false otherwise
 */
bool readStringList(QXmlStreamReader& xml, const char* const currentElement,
                    const QString& stringListElement,
                    const QString& listEntryElementName,
                    QStringList& list, StringCache &strCache);

void writeStringList(QXmlStreamWriter& xml, const QStringList& list, const QString& stringListElement, const QString& listEntryElementName, bool sort = false);

void writeStringListHash(QXmlStreamWriter& xml,
                         const QHash<QString, QStringList>& hash,
                         const QString& hashName,
                         const QString& hashEntryName,
                         const QString& hashKeyAttributeName,
                         const QString& stringElementName,
                         bool sortString);
bool readStringListHash(QXmlStreamReader& xml, const char* const currentElement,
                        const QString& hashName,
                        const QString& hashEntryName,
                        const QString& hashKeyName,
                        const QString& stringElementName,
                        QHash<QString, QStringList>& hash,
                        StringCache &strCache);

/**
 * @brief readGeneralList read a general list
 *
 * The string list is expected to be in the following form:
 * <listElement>
 *   <listEntryElementName>...</listEntryElementName>
 *   ...
 *   <listEntryElementName>...</listEntryElementName>
 * </listElement>
 *
 * @param xml the xml must be before the StartElement of listElement
 * @param currentElement a string representing the current element / class (for logging purpose only)
 * @param listElement the xml element name for the general list
 * @param listEntryElementName the xml element name for list entry
 * @param readElementCallback the callback for reading the child entry; xml will be at StartElement upon call
 * @param strCache the string cache
 * @return true on success, false otherwise
 */
bool readGeneralList(QXmlStreamReader& xml, const char* const currentElement,
                     const QString& listElement,
                     const QString& listEntryElementName,
                     std::function<bool(QXmlStreamReader&, StringCache&)> readElementCallback,
                     StringCache &strCache);

bool readElementText(QXmlStreamReader& xml, const char* const currentElement, const QString &textElement, QString& str, StringCache &strCache);

void writeFlagElement(QXmlStreamWriter& xml, std::initializer_list<std::pair<bool, QString>> flags, const QString& flagElementName);
bool readFlagElement(QXmlStreamReader& xml, const char* const currentElement, std::initializer_list<std::pair<bool&, QString>> flags, const QString& flagElementName, StringCache &strCache);
}

namespace XMLError {


inline void errorCommon(QDebug& msg, QXmlStreamReader& xml, const char* const currentElement)
{
    msg.noquote().nospace() << "At XML line " << xml.lineNumber()
                            << ", column " << xml.columnNumber()
                            << ": when reading " << currentElement << ": ";
}

inline bool missingStartElement(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                                const QString& missingElement)
{
    errorCommon(msg, xml, currentElement);
    msg << "StartElement for " << missingElement << " is not found";
    return false;
}

inline bool unexpectedElement(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                              const QString& expectedElement)
{
    errorCommon(msg, xml, currentElement);
    msg << "Unexpected xml element " << xml.name() << " (Expecting " << expectedElement <<")";
    return false;
}

inline bool notHavingCharacter(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                               const QString& childElement)
{
    errorCommon(msg, xml, currentElement);
    msg << "Unexpected xml token " << xml.tokenString() << " (Expecting Character for " << childElement << ")";
    return false;
}

inline bool invalidEnumString(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                              const QString& childElementOrAttribute, std::initializer_list<QString> legalValues)
{
    errorCommon(msg, xml, currentElement);
    msg << "Invalid enum \"" << xml.text() << "\" for " << childElementOrAttribute;
    if (legalValues.begin() != legalValues.end()) {
        msg <<" (valid values are {";
        bool isFirst = true;
        for (const auto& str : legalValues) {
            if (!isFirst) {
                msg << ", ";
            } else {
                isFirst = false;
            }
            msg << str;
        }
        msg << "})";
    }
    return false;
}

inline bool invalidValue(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                         const QString& valueName, QStringRef value, const char* const reason)
{
    errorCommon(msg, xml, currentElement);
    msg << "Invalid value \"" << value << "\" for " << valueName;
    if (reason) {
        msg << " (Reason: " << reason << ")";
    }
    return false;
}

inline bool missingEndElement(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                              const QString& childElement)
{
    errorCommon(msg, xml, currentElement);
    msg << "When wrapping up " << childElement << ": unexpected xml token " << xml.tokenString() << " (Expecting EndElement)";
    return false;
}

inline bool missingAttribute(QDebug&& msg, QXmlStreamReader& xml, const char* const currentElement,
                      QStringRef childElement, const QString& attr)
{
    errorCommon(msg, xml, currentElement);
    msg << "Missing attribute " << attr << " on element " << childElement;
    return false;
}

inline bool failOnChild(QDebug&& msg, const char* const currentElement, const QString& childElement)
{
    msg.noquote().nospace() << "Failed to read " << currentElement << " because reading child element " << childElement << " failed";
    return false;
}
}

namespace XMLUtil {
template <typename Loadable>
bool readLoadable(QXmlStreamReader& xml, const char* const currentElement,
                  const QString& elementName, Loadable& child, StringCache& strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, elementName);
    }
    if (Q_UNLIKELY(xml.name() != elementName)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, elementName);
    }
    if (Q_UNLIKELY(!child.loadFromXML(xml, strCache))) {
        return XMLError::failOnChild(qWarning(), currentElement, elementName);
    }
    Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    return true;
}

template <typename ListOfLoadable>
bool readLoadableList(QXmlStreamReader& xml, const char* const currentElement,
                     const QString& listElement,
                     const QString& listEntryElementName,
                     ListOfLoadable& list,
                     StringCache& strCache)
{
    if (Q_UNLIKELY(!xml.readNextStartElement())) {
        return XMLError::missingStartElement(qWarning(), xml, currentElement, listElement);
    }
    if (Q_UNLIKELY(xml.name() != listElement)) {
        return XMLError::unexpectedElement(qWarning(), xml, currentElement, listElement);
    }
    list.clear();
    while (xml.readNextStartElement()) {
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::StartElement);
        if (Q_UNLIKELY(xml.name() != listEntryElementName)) {
            return XMLError::unexpectedElement(qWarning(), xml, currentElement, listEntryElementName);
        }
        typename std::remove_reference<decltype (list.front())>::type dummy;
        list.push_back(dummy);
        if (Q_UNLIKELY(!list.back().loadFromXML(xml, strCache))) {
            return XMLError::failOnChild(qWarning(), currentElement, listEntryElementName);
        }
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, listElement);
    }
    return true;
}

template <typename ListOfLoadable>
void writeLoadableList(QXmlStreamWriter& xml, const ListOfLoadable& list,
                       const QString& listElement,
                       const QString& listEntryElementName)
{
    xml.writeStartElement(listElement);
    for (const auto& element : list) {
        xml.writeStartElement(listEntryElementName);
        element.saveToXML(xml);
    }
    xml.writeEndElement();
}

template <typename Loadable>
bool readLoadableHash(
        QXmlStreamReader& xml, const char* const currentElement,
        const QString& hashName,
        const QString& hashEntryName,
        const QString& hashKeyName,
        QHash<QString, Loadable>& hash,
        StringCache& strCache)
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
        Loadable dummy;
        auto iter = hash.insert(key, dummy);
        if (Q_UNLIKELY(!iter.value().loadFromXML(xml, strCache))) {
            return XMLError::failOnChild(qWarning(), currentElement, hashEntryName);
        }
        Q_ASSERT(xml.tokenType() == QXmlStreamReader::EndElement);
    }
    if (Q_UNLIKELY(xml.tokenType() != QXmlStreamReader::EndElement)) {
        return XMLError::missingEndElement(qWarning(), xml, currentElement, hashName);
    }
    return true;
}

template <typename Loadable>
void writeLoadableHash(
        QXmlStreamWriter& xml,
        const QHash<QString, Loadable>& hash,
        const QString& hashName,
        const QString& hashEntryName,
        const QString& hashKeyName)
{
    xml.writeStartElement(hashName);
    QStringList keys = hash.keys();
    NameSorting::sortNameList(keys);
    for (const auto& key : keys) {
        auto iter = hash.find(key);
        xml.writeStartElement(hashEntryName);
        xml.writeAttribute(hashKeyName, key);
        iter.value().saveToXML(xml);
    }
    xml.writeEndElement();
}
} // namespace XMLUtil


#endif // STRINGCACHE_H
