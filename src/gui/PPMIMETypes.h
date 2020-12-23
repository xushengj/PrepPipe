#ifndef PPMIMETYPES_H
#define PPMIMETYPES_H

#include <QString>

#ifdef PPMIMETYPE_CPP
#include <QStringLiteral>
#define MIMEDECL(name, content) const QString name = QStringLiteral(content)
#else
#define MIMEDECL(name, content) const QString name
#endif

namespace PP_MIMETYPE {

// https://stackoverflow.com/questions/3205244/mime-type-conventions-standards-or-limitations

MIMEDECL(ObjectContext_ObjectReference,         "application/x.preppipe.object-ref");
MIMEDECL(SimpleParser_RuleNodeReference,        "application/x.preppipe.simpleparser-rule-ref");

}

#undef MIMEDECL
#endif // PPMIMETYPES_H
