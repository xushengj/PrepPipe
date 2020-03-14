#ifndef CONFIGURATIONINPUTWIDGET_H
#define CONFIGURATIONINPUTWIDGET_H

#include "src/lib/Tree/Configuration.h"

#include <QWidget>
#include <QLabel>
#include <QFormLayout>

class ConfigurationInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigurationInputWidget(QWidget *parent = nullptr);
    virtual ~ConfigurationInputWidget() override;

    void setConfigurationDeclaration(const ConfigurationDeclaration* decl);
    bool getConfigurationData(ConfigurationData& result, QHash<int, int> &fieldIndexToNodeIndexMap);
    bool getConfigurationData(ConfigurationData& result);

signals:

private:
    struct FieldData {
        bool isEnabled = false;
        QLabel* label = nullptr; // nullptr if this is a group

        // input widget:
        // QComboBox for enum
        // QCheckBox for boolean
        // QGroupBox for group
        // QLineEdit for everything else
        QWidget* inputWidget = nullptr;
        QFormLayout* parentLayout = nullptr;
    };

private slots:
    void refreshForm();
private:
    const ConfigurationDeclaration* configDecl = nullptr;
    QFormLayout* rootLayout = nullptr;
    QVector<FieldData> fieldData;
};

#endif // CONFIGURATIONINPUTWIDGET_H
