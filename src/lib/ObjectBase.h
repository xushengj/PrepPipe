#ifndef OBJECTBASE_H
#define OBJECTBASE_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QMetaEnum>
#include <QIODevice>

#ifndef PP_DISABLE_GUI
#include <QIcon>
#include <QMenu>
#endif

#include <functional>

class ObjectBase : public QObject
{
    Q_OBJECT

public:
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

    // this enum determines the sort order among object types
    // any value with all-capital name should not be used on object instances
    enum ObjectType {
        INVALID,

        TASK_START,
        Task_SimpleParser,
        Task_SimpleTextGenerator,
        Task_SimpleTreeTransform,
        Task_SimpleWorkflow,
        Task_Test,
        TASK_END,

        DATA_START,
        Data_GeneralTree,
        Data_PlainText,
        Data_MIME,
        DATA_END,

        EXEC_START,
        Exec_SimpleParser,
        Exec_SimpleTextGenerator,
        Exec_SimpleTreeTransform,
        Exec_SimpleWorkflow,
        Exec_Test,
        EXEC_END
    };
    Q_ENUM(ObjectType)

    explicit ObjectBase(ObjectType Ty);
    ObjectBase(ObjectType Ty, const QString& name);
    ObjectBase(const ObjectBase& src);
    virtual ~ObjectBase() {}
    virtual ObjectBase* clone() = 0;

    static QString getTypeClassName(ObjectType ty);
    static QString getTypeDisplayName(ObjectType ty);
#ifndef PP_DISABLE_GUI
    static QIcon   getTypeDisplayIcon(ObjectType ty);
#endif

    ObjectType  getType()               const {return ty;}
    QString     getTypeClassName()      const {return getTypeClassName(ty);}
    QString     getTypeDisplayName()    const {return getTypeDisplayName(ty);}

#ifndef PP_DISABLE_GUI
    QIcon       getTypeDisplayIcon()    const {return getTypeDisplayIcon(ty);}
#endif

#ifndef PP_DISABLE_GUI
    // for use in EditorWindow
    // for objects not backed by a file, the editor can always be destroyed
    // virtual function asking for dirty editor data is in FileBackedObject
    virtual QWidget* getEditor() {return nullptr;}
    virtual void tearDownEditor(QWidget* editor) {editor->hide(); delete editor;}

    // for use in ExecuteWindow
    // viewers should be able to get destructed properly by just invoking destructor
    virtual QWidget* getViewer() {return nullptr;}
#endif

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
        name = newName;
    }

    void setNameSpace(const QStringList& newNamespace) {
        nameSpace = newNamespace;
    }

    void setComment(const QString& newComment) {
        comment = newComment;
    }

private:
    QString name;
    QString comment;

    QStringList nameSpace;
    const ObjectType ty;
};

#endif // OBJECTBASE_H
