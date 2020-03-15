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
#include <QMenu>

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

    struct NamedReference {
        QString name;
        QStringList nameSpace;
        NamedReference() = default;
        NamedReference(QString nameArg, QStringList nameSpaceArg)
            : name(nameArg), nameSpace(nameSpaceArg)
        {}
        NamedReference(QString expr);
        QString prettyPrint() const;
        QString prettyPrint(const QStringList& mainNameSpace) const;
    };

    static QString prettyPrintNameSpace(const QStringList& nameSpace);

    enum ObjectType {
        Data_GeneralTree,
        Data_PlainText,
        Data_MIME,
        Task_SimpleTreeTransform,
        Exec_SimpleTreeTransform,
        Task_Test,
        Exec_Test
    };
    Q_ENUM(ObjectType)

    explicit ObjectBase(ObjectType Ty);
    ObjectBase(ObjectType Ty, const QString& name);
    ObjectBase(const ObjectBase& src);
    virtual ~ObjectBase() {}
    virtual ObjectBase* clone() = 0;

    static QString getTypeClassName(ObjectType ty);
    static QString getTypeDisplayName(ObjectType ty);
    static QIcon   getTypeDisplayIcon(ObjectType ty);

    ObjectType  getType()               const {return ty;}
    QString     getTypeClassName()      const {return getTypeClassName(ty);}
    QString     getTypeDisplayName()    const {return getTypeDisplayName(ty);}
    QIcon       getTypeDisplayIcon()    const {return getTypeDisplayIcon(ty);}

    // for objects not backed by a file, the editor can always be destroyed
    // virtual function asking for dirty editor data is in FileBackedObject
    virtual QWidget* getEditor() {return nullptr;}
    virtual void tearDownEditor(QWidget* editor) {editor->hide(); delete editor;}

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

    virtual QString getFilePath() const {return QString();} // only overriden in FileBackedObject

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

    void setStatus(StatusFlags stat) {
        status = stat;
    }

    // helpers
    void lock() {
        status |= StatusFlag::Locked;
    }

signals:
    void statusChanged(StatusFlags stat);

private:
    QString name;
    QString comment;

    QStringList nameSpace;
    const ObjectType ty;
    StatusFlags status;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ObjectBase::StatusFlags)

#endif // OBJECTBASE_H
