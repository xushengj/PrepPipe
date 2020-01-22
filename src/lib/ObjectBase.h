#ifndef OBJECTBASE_H
#define OBJECTBASE_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QMetaEnum>
#include <QIcon>
#include <QIODevice>

#include <functional>

class ObjectBase : public QObject
{
    Q_OBJECT

public:
    enum StatusFlag {
        None        = 0x00,
        Locked      = 0x01, //!< The object is read-only; users must fork a writeable copy before editing. This cannot be changed after the object is constructed.
        Unbacked    = 0x02, //!< The object is currently not backed by a file (not subject to single-object-per-file check)
        Invalid     = 0x04  //!< The object is in invalid state and: 1. gui should prompt user to edit it; 2. this object cannot participate in activities
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)


    enum ObjectType { // this determines the sort order in UI
        GeneralTree = 0
    };
    Q_ENUM(ObjectType)

    struct ConstructOptions {
        // self contained for intrinsic object
        QString name;
        QString comment;
        // from environment / settings
        QString filePath;
        QStringList nameSpace;
        bool isLocked;
    };

    ObjectBase(ObjectType Ty, const ConstructOptions& opt);
    virtual ~ObjectBase() {}

    static QString getTypeClassName(ObjectType ty);
    static QString getTypeDisplayName(ObjectType ty);
    static QIcon   getTypeDisplayIcon(ObjectType ty);

    ObjectType  getType()               const {return ty;}
    QString     getTypeClassName()      const {return getTypeClassName(ty);}
    QString     getTypeDisplayName()    const {return getTypeDisplayName(ty);}
    QIcon       getTypeDisplayIcon()    const {return getTypeDisplayIcon(ty);}

    virtual QWidget* getEditor() {return nullptr;}
    virtual bool editorOkayToClose(QWidget* editor, QWidget* window) {Q_UNUSED(editor) Q_UNUSED(window) return true;}

    StatusFlags getStatus() const {return status;}

    QString getName() const {
        return name;
    }

    QStringList getNameSpace() const {
        return nameSpace;
    }

    QString getComment() const {
        return comment;
    }

    QString getFilePath() const {
        return filePath;
    }

    void setName(const QString& newName) {
        Q_ASSERT(!(getStatus() & StatusFlag::Locked));
        name = newName;
    }

    void setNameSpace(const QStringList& newNamespace) {
        Q_ASSERT(!(getStatus() & StatusFlag::Locked));
        nameSpace = newNamespace;
    }

    void setComment(const QString& newComment) {
        Q_ASSERT(!(getStatus() & StatusFlag::Locked));
        comment = newComment;
    }

    void setFilePath(const QString& newFilePath) {
        filePath = newFilePath;
    }

signals:
    void statusChanged(StatusFlags stat);

private:
    QString name;
    QString comment;
    QString filePath;
    QStringList nameSpace;
    const ObjectType ty;
    StatusFlags status;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ObjectBase::StatusFlags)

#endif // OBJECTBASE_H
