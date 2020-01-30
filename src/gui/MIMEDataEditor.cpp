#include "MIMEDataEditor.h"
#include "ui_MIMEDataEditor.h"
#include "src/lib/DataObject/MIMEDataObject.h"

#include <QtGlobal>
#include <QGuiApplication>
#include <QClipboard>

MIMEDataEditor::MIMEDataEditor(MIMEDataObject *srcPtr) :
    QWidget(nullptr),
    ui(new Ui::MIMEDataEditor),
    src(srcPtr)
{
    ui->setupUi(this);
    ui->plainTextEdit->setReadOnly(true);

    refreshDataFromSrc();
    QFont font = ui->plainTextEdit->font();
    font.setFixedPitch(true);
    font.setStyleHint(QFont::TypeWriter);
    ui->plainTextEdit->setFont(font);
    connect(ui->formatComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MIMEDataEditor::currentFormatChanged);
    connect(ui->copyPushButton, &QPushButton::clicked, this, &MIMEDataEditor::copyToClipBoard);
}

MIMEDataEditor::~MIMEDataEditor()
{
    delete ui;
}

void MIMEDataEditor::copyToClipBoard()
{
    QMimeData* ptr = new QMimeData;
    const auto& data = src->getData();
    for (auto iter = data.begin(), iterEnd = data.end(); iter != iterEnd; ++iter) {
        ptr->setData(iter.key(), iter.value());
    }
    QGuiApplication::clipboard()->setMimeData(ptr, QClipboard::Clipboard);
}

void MIMEDataEditor::refreshDataFromSrc()
{
    // reset
    ui->plainTextEdit->clear();
    ui->formatComboBox->setEnabled(false);
    ui->formatComboBox->clear();
    ui->viewListWidget->clear();
    ui->addViewPushButton->setEnabled(false);
    formatDataList.clear();

    // populate from data
    const auto& data = src->getData();
    if (data.isEmpty()) {
        ui->formatComboBox->addItem(tr("(No data)"));
        return;
    }

    int formatCnt = data.size();
    formatDataList.reserve(formatCnt);
    for (auto iter = data.begin(), iterEnd = data.end(); iter != iterEnd; ++iter) {
        ui->formatComboBox->addItem(iter.key());
        FormatData entry;
        entry.data = iter.value();
        entry.viewList.push_back(tr("Binary"));
        entry.viewDataList.push_back(getBinaryView(entry.data));
        entry.currentView = 0;
        formatDataList.push_back(entry);
    }
    ui->formatComboBox->setEnabled(true);

    currentFormatChanged(0);
}

QString MIMEDataEditor::getBinaryView(const QByteArray& data)
{
    QString buf;
    int rowCnt = (data.size() / 16) + 1;
    int addrColumnWidth = 1;
    {
        int tmpRowCnt = rowCnt;
        while (tmpRowCnt > 0) {
            addrColumnWidth += 1;
            tmpRowCnt >>= 4;
        }
    }
    auto getChar = [](int value) -> char {
        if (value < 10) {
            return static_cast<char>('0' + value);
        } else {
            return static_cast<char>('a' + value - 10);
        }
    };
    buf.reserve(rowCnt * (addrColumnWidth + 3 + 16 * 3 + 11));
    for (int rowIndex = 0; rowIndex < rowCnt; ++rowIndex) {
        // addr
        QString digits = QString::number(rowIndex, 16);
        int missingPrecedingZero = addrColumnWidth - 1 - digits.size();
        Q_ASSERT(missingPrecedingZero >= 0);
        if (missingPrecedingZero > 0) {
            buf.append(QString(missingPrecedingZero, '0'));
        }
        buf.append(digits);
        buf.append("0 | ");
        // hex
        int base = rowIndex * 16;
        if (base >= data.size())
            break;
        int bound = base + 16;
        if (bound > data.size()) {
            bound = data.size();
        }
        QString chars;
        chars.reserve(bound - base);
        for (int i = base; i < bound; ++i) {
            char c = data.at(i);
            if (c >= ' ' && c <= 0x7e) {
                chars.push_back(c);
            } else {
                chars.push_back('.');
            }
            buf.append(' ');
            buf.append(getChar((c >> 4) & 0x0f));
            buf.append(getChar(c & 0x0f));
        }
        int missingSet = 16 - (bound - base);
        if (missingSet > 0) {
            buf.append(QString(missingSet * 3, ' '));
        }
        buf.append(" | ");
        buf.append(chars);
        buf.append('\n');
    }
    return buf;
}

void MIMEDataEditor::currentFormatChanged(int formatIndex)
{
    if (formatIndex == -1 || formatIndex >= formatDataList.size())
        return;

    ui->viewListWidget->clear();
    const auto& data = formatDataList.at(formatIndex);
    Q_ASSERT(data.viewList.size() > 0);
    ui->viewListWidget->addItems(data.viewList);
    ui->viewListWidget->setCurrentRow(0);
    ui->plainTextEdit->setPlainText(data.viewDataList.front());
}
