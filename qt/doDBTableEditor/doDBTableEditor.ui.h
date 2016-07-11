/********************************************************************************
** Form generated from reading UI file 'doDBTableEditor.ui'
**
** Created by: Qt User Interface Compiler version 5.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DODBTABLEEDITOR_H
#define DODBTABLEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_doDBTableEditorUi
{
public:
    QVBoxLayout *verticalLayout_2;
    QGroupBox *tablesBox;
    QHBoxLayout *horizontalLayout_3;
    QComboBox *tablesCBox;
    QPushButton *btnTableDelete;
    QPushButton *btnTableNew;
    QGroupBox *groupTable;
    QHBoxLayout *horizontalLayout_4;
    QLabel *tableNameLabel;
    QLineEdit *tableName;
    QLabel *tableDisplayNameLabel;
    QLineEdit *tableDisplayName;
    QPushButton *btnTableSave;
    QGroupBox *groupColumns;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout;
    QTableWidget *listColumns;
    QVBoxLayout *verticalLayout_4;
    QPushButton *btnColumnAdd;
    QPushButton *btnColumnDelete;
    QSpacerItem *verticalSpacer_2;
    QGroupBox *groupColumn;
    QGridLayout *gridLayout;
    QLabel *tableColumnTypeLabel;
    QComboBox *columnType;
    QCheckBox *columnOptUnique;
    QLabel *tableColumnNameLabel;
    QCheckBox *columnOptPrimaryKey;
    QLineEdit *columnName;
    QPushButton *btnColumnSave;
    QLineEdit *columnDisplayName;
    QLabel *label;
    QCheckBox *columnOptNotNull;
    QCheckBox *ckBoxDisplayColumn;
    QPushButton *btnTableCreate;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnClose;

    void setupUi(QWidget *doDBTableEditorUi)
    {
        if (doDBTableEditorUi->objectName().isEmpty())
            doDBTableEditorUi->setObjectName(QStringLiteral("doDBTableEditorUi"));
        doDBTableEditorUi->resize(849, 708);
        verticalLayout_2 = new QVBoxLayout(doDBTableEditorUi);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        tablesBox = new QGroupBox(doDBTableEditorUi);
        tablesBox->setObjectName(QStringLiteral("tablesBox"));
        horizontalLayout_3 = new QHBoxLayout(tablesBox);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        tablesCBox = new QComboBox(tablesBox);
        tablesCBox->setObjectName(QStringLiteral("tablesCBox"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tablesCBox->sizePolicy().hasHeightForWidth());
        tablesCBox->setSizePolicy(sizePolicy);

        horizontalLayout_3->addWidget(tablesCBox);

        btnTableDelete = new QPushButton(tablesBox);
        btnTableDelete->setObjectName(QStringLiteral("btnTableDelete"));

        horizontalLayout_3->addWidget(btnTableDelete);

        btnTableNew = new QPushButton(tablesBox);
        btnTableNew->setObjectName(QStringLiteral("btnTableNew"));

        horizontalLayout_3->addWidget(btnTableNew);


        verticalLayout_2->addWidget(tablesBox);

        groupTable = new QGroupBox(doDBTableEditorUi);
        groupTable->setObjectName(QStringLiteral("groupTable"));
        horizontalLayout_4 = new QHBoxLayout(groupTable);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        tableNameLabel = new QLabel(groupTable);
        tableNameLabel->setObjectName(QStringLiteral("tableNameLabel"));

        horizontalLayout_4->addWidget(tableNameLabel);

        tableName = new QLineEdit(groupTable);
        tableName->setObjectName(QStringLiteral("tableName"));

        horizontalLayout_4->addWidget(tableName);

        tableDisplayNameLabel = new QLabel(groupTable);
        tableDisplayNameLabel->setObjectName(QStringLiteral("tableDisplayNameLabel"));

        horizontalLayout_4->addWidget(tableDisplayNameLabel);

        tableDisplayName = new QLineEdit(groupTable);
        tableDisplayName->setObjectName(QStringLiteral("tableDisplayName"));

        horizontalLayout_4->addWidget(tableDisplayName);

        btnTableSave = new QPushButton(groupTable);
        btnTableSave->setObjectName(QStringLiteral("btnTableSave"));

        horizontalLayout_4->addWidget(btnTableSave);


        verticalLayout_2->addWidget(groupTable);

        groupColumns = new QGroupBox(doDBTableEditorUi);
        groupColumns->setObjectName(QStringLiteral("groupColumns"));
        verticalLayout_6 = new QVBoxLayout(groupColumns);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        listColumns = new QTableWidget(groupColumns);
        listColumns->setObjectName(QStringLiteral("listColumns"));
        listColumns->horizontalHeader()->setStretchLastSection(true);

        horizontalLayout->addWidget(listColumns);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        btnColumnAdd = new QPushButton(groupColumns);
        btnColumnAdd->setObjectName(QStringLiteral("btnColumnAdd"));

        verticalLayout_4->addWidget(btnColumnAdd);

        btnColumnDelete = new QPushButton(groupColumns);
        btnColumnDelete->setObjectName(QStringLiteral("btnColumnDelete"));
        btnColumnDelete->setEnabled(false);

        verticalLayout_4->addWidget(btnColumnDelete);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_2);


        horizontalLayout->addLayout(verticalLayout_4);


        verticalLayout_6->addLayout(horizontalLayout);


        verticalLayout_2->addWidget(groupColumns);

        groupColumn = new QGroupBox(doDBTableEditorUi);
        groupColumn->setObjectName(QStringLiteral("groupColumn"));
        gridLayout = new QGridLayout(groupColumn);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        tableColumnTypeLabel = new QLabel(groupColumn);
        tableColumnTypeLabel->setObjectName(QStringLiteral("tableColumnTypeLabel"));

        gridLayout->addWidget(tableColumnTypeLabel, 0, 4, 1, 1);

        columnType = new QComboBox(groupColumn);
        columnType->setObjectName(QStringLiteral("columnType"));
        columnType->setEditable(false);

        gridLayout->addWidget(columnType, 0, 5, 1, 1);

        columnOptUnique = new QCheckBox(groupColumn);
        columnOptUnique->setObjectName(QStringLiteral("columnOptUnique"));

        gridLayout->addWidget(columnOptUnique, 0, 8, 1, 1);

        tableColumnNameLabel = new QLabel(groupColumn);
        tableColumnNameLabel->setObjectName(QStringLiteral("tableColumnNameLabel"));

        gridLayout->addWidget(tableColumnNameLabel, 0, 0, 1, 1);

        columnOptPrimaryKey = new QCheckBox(groupColumn);
        columnOptPrimaryKey->setObjectName(QStringLiteral("columnOptPrimaryKey"));

        gridLayout->addWidget(columnOptPrimaryKey, 0, 6, 1, 1);

        columnName = new QLineEdit(groupColumn);
        columnName->setObjectName(QStringLiteral("columnName"));

        gridLayout->addWidget(columnName, 0, 1, 1, 1);

        btnColumnSave = new QPushButton(groupColumn);
        btnColumnSave->setObjectName(QStringLiteral("btnColumnSave"));
        btnColumnSave->setEnabled(true);

        gridLayout->addWidget(btnColumnSave, 0, 9, 1, 1);

        columnDisplayName = new QLineEdit(groupColumn);
        columnDisplayName->setObjectName(QStringLiteral("columnDisplayName"));

        gridLayout->addWidget(columnDisplayName, 1, 1, 1, 1);

        label = new QLabel(groupColumn);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 1, 0, 1, 1);

        columnOptNotNull = new QCheckBox(groupColumn);
        columnOptNotNull->setObjectName(QStringLiteral("columnOptNotNull"));

        gridLayout->addWidget(columnOptNotNull, 1, 6, 1, 1);

        ckBoxDisplayColumn = new QCheckBox(groupColumn);
        ckBoxDisplayColumn->setObjectName(QStringLiteral("ckBoxDisplayColumn"));

        gridLayout->addWidget(ckBoxDisplayColumn, 1, 8, 1, 1);


        verticalLayout_2->addWidget(groupColumn);

        btnTableCreate = new QPushButton(doDBTableEditorUi);
        btnTableCreate->setObjectName(QStringLiteral("btnTableCreate"));

        verticalLayout_2->addWidget(btnTableCreate);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        btnClose = new QPushButton(doDBTableEditorUi);
        btnClose->setObjectName(QStringLiteral("btnClose"));

        horizontalLayout_2->addWidget(btnClose);


        verticalLayout_2->addLayout(horizontalLayout_2);


        retranslateUi(doDBTableEditorUi);

        QMetaObject::connectSlotsByName(doDBTableEditorUi);
    } // setupUi

    void retranslateUi(QWidget *doDBTableEditorUi)
    {
        doDBTableEditorUi->setWindowTitle(QApplication::translate("doDBTableEditorUi", "Form", 0));
        tablesBox->setTitle(QApplication::translate("doDBTableEditorUi", "Tabellen", 0));
        btnTableDelete->setText(QApplication::translate("doDBTableEditorUi", "L\303\266schen", 0));
        btnTableNew->setText(QApplication::translate("doDBTableEditorUi", "Neu", 0));
        groupTable->setTitle(QApplication::translate("doDBTableEditorUi", "Ausgew\303\244hlte Tabelle", 0));
        tableNameLabel->setText(QApplication::translate("doDBTableEditorUi", "Name", 0));
        tableDisplayNameLabel->setText(QApplication::translate("doDBTableEditorUi", "Anzeigename", 0));
        btnTableSave->setText(QApplication::translate("doDBTableEditorUi", "Speichern", 0));
        groupColumns->setTitle(QApplication::translate("doDBTableEditorUi", "Spalten", 0));
        btnColumnAdd->setText(QApplication::translate("doDBTableEditorUi", "Hinzuf\303\274gen", 0));
        btnColumnDelete->setText(QApplication::translate("doDBTableEditorUi", "L\303\266schen", 0));
        groupColumn->setTitle(QApplication::translate("doDBTableEditorUi", "Spalte", 0));
        tableColumnTypeLabel->setText(QApplication::translate("doDBTableEditorUi", "Type", 0));
        columnOptUnique->setText(QApplication::translate("doDBTableEditorUi", "Unique", 0));
        tableColumnNameLabel->setText(QApplication::translate("doDBTableEditorUi", "Name", 0));
        columnOptPrimaryKey->setText(QApplication::translate("doDBTableEditorUi", "Primary Key", 0));
        btnColumnSave->setText(QApplication::translate("doDBTableEditorUi", "Speichern", 0));
        label->setText(QApplication::translate("doDBTableEditorUi", "AnzeigeName", 0));
        columnOptNotNull->setText(QApplication::translate("doDBTableEditorUi", "Not Null", 0));
        ckBoxDisplayColumn->setText(QApplication::translate("doDBTableEditorUi", "Anzeigespalte", 0));
        btnTableCreate->setText(QApplication::translate("doDBTableEditorUi", "Tabelle in Datenbank erzeugen", 0));
        btnClose->setText(QApplication::translate("doDBTableEditorUi", "Schlie\303\237en", 0));
    } // retranslateUi

};

namespace Ui {
    class doDBTableEditorUi: public Ui_doDBTableEditorUi {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DODBTABLEEDITOR_H
