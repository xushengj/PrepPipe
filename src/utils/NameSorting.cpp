#include "NameSorting.h"

#include <algorithm>

QCollator* NameSorting::collator = nullptr;

void NameSorting::init()
{
    collator = new QCollator(QLocale());
}
