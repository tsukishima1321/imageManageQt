#include "dbwidget.h"
#include "ui_dbwidget.h"
#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QSettings>

dbWidget::dbWidget(QWidget *parent, QList<Item *> *itemList, QString name, QString password) :
    QWidget(parent),
    ui(new Ui::dbWidget)
{
    ui->setupUi(this);
    qDebug() << QSqlDatabase::drivers();
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName("diary");
    db.setUserName(name);
    db.setPassword(password);
    db.setPort(3306);
    if(!db.open()){
        //qDebug()<<"Database Connect Failed";
        QMessageBox::critical(this, "ERROR", "Database Connection Failed");
        delete this;
    }else{
        //qDebug()<<"database connected";
        typeList.clear();
        this->itemList=itemList;
        updateTable2();
        this->show();
        ui->status->setText("开始更新分类列表");
        QString sql = "SELECT * FROM types";
        QSqlQuery query(db);
        query.prepare(sql);
        query.exec();
        while (query.next()) {
            QString type=query.value("typename").toString();
            typeList.push_back(type);
        }
        ui->status->setText("分类列表更新完成");
    }
}

void dbWidget::updateTable1(){
    QString date=ui->dateEdit->text();
    QString sql = "SELECT * FROM pictures WHERE date='" + date + "'";
    QSqlQuery query(db);
    query.exec(sql);
    QList<Item*> items;
    items.clear();
    while (query.next()) {
        items.push_back(new Item(query.value("date").toString(),query.value("href").toString(),query.value("description").toString(),query.value("type").toString()));
    }
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(items.length());
    int i=0;
    for(Item* item : items){
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(item->date));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(item->href));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(item->description));
        ui->tableWidget->setItem(i,3,new QTableWidgetItem(item->type));
        i++;
    }
}

void dbWidget::updateTable2(){
    int i=0;
    ui->tableWidget_2->clearContents();
    ui->tableWidget_2->setRowCount(itemList->length());
    for(Item* item : *(this->itemList)){
        ui->tableWidget_2->setItem(i,0,new QTableWidgetItem(item->date));
        ui->tableWidget_2->setItem(i,1,new QTableWidgetItem(item->href));
        ui->tableWidget_2->setItem(i,2,new QTableWidgetItem(item->description));
        ui->tableWidget_2->setItem(i,3,new QTableWidgetItem(item->type));
        i++;
    }
}

dbWidget::~dbWidget()
{
    delete ui;
}

void dbWidget::on_searchButton_clicked()
{
    updateTable1();
}


void dbWidget::on_addallButton_clicked()
{
    ui->status->setText("正在添加");
    QSqlQuery query(db);
    query.prepare("INSERT INTO pictures (date,href,description,type) VALUES (:date,:href,:description,:type)");
    for(Item* item : *(this->itemList)){
        qDebug()<<"23456543";
        query.bindValue(":date",item->date);
        query.bindValue(":href",item->href);
        query.bindValue(":description",item->description);
        query.bindValue(":type",item->type);
        query.exec();
    }
    itemList->clear();
    ui->tableWidget_2->clearContents();
    ui->status->setText("添加完成");
}


void dbWidget::on_addButton_2_clicked()
{
    ui->status->setText("正在添加");
    int i=ui->tableWidget_2->currentRow();
    QSqlQuery query(db);
    query.prepare("INSERT INTO pictures (date,href,description,type) VALUES (:date,:href,:description,:type)");
    Item* item=(*itemList)[i];
    query.bindValue(":date",item->date);
    query.bindValue(":href",item->href);
    query.bindValue(":description",item->description);
    query.bindValue(":type",item->type);
    query.exec();
    itemList->removeAt(i);
    updateTable2();
    updateTable1();
    ui->status->setText("添加完成");
}


void dbWidget::on_deleteButton_clicked()
{
    ui->status->setText("正在删除");
    int i=ui->tableWidget->currentRow();
    QSqlQuery query(db);
    query.prepare("delete from pictures where href=:href");
    query.bindValue(":href",ui->tableWidget->item(i,1)->text());
    query.exec();
    updateTable1();
    ui->status->setText("删除完成");
}


void dbWidget::on_buildButton_clicked()
{
    ui->status->setText("正在构建");
    QFile file("图片索引.htm");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QString text="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n<html>\n<head>\n<title>图片索引</title>\n<meta name=\"GENERATOR\" content=\"WinCHM\">\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">\n<style>\nhtml,body { \n	font-family: Arial, Helvetica, sans-serif;\n	font-size: 11pt;\n}\n</style>\n</head>\n<body>\n";
        file.write(text.toLocal8Bit().data());
        //QString sql = "SELECT * FROM pictures WHERE type=:type ORDER BY date ASC,description ASC";
        QString sql = "SELECT * FROM pictures WHERE type=:type ORDER BY date ASC";
        QSqlQuery query(db);
        query.prepare(sql);
        for(const QString& type:typeList){
            text="<P>\n<FONT size=3><STRONG>"+type+"</STRONG></FONT>\n</P>\n<P>\n";
            file.write(text.toLocal8Bit().data());
            query.bindValue(":type",type);
            query.exec();
            while (query.next()) {
                text="<A href=\""+query.value("href").toString()+"\">"+query.value("description").toString()+"</A>&nbsp;——"+query.value("date").toString()+"<BR>"+"\n";
                file.write(text.toLocal8Bit().data());
            }
            text="</P>\n";
            file.write(text.toLocal8Bit().data());
        }
        text="</body>\n</html>";
        file.write(text.toLocal8Bit().data());
        file.close();
    }
    ui->status->setText("构建完成");
}


void dbWidget::on_pushButton_clicked()
{
    QString sql = ui->sqlEdit->text();
    QString warning="";
    if(sql.contains("insert")){
        warning+="insert ";
    }
    if(sql.contains("delete")){
        warning+="delete ";
    }
    if(warning!=""){
        QMessageBox message(QMessageBox::Warning, "警告", "含有危险操作:"+warning+"\n是否继续？", QMessageBox::Yes | QMessageBox::No, NULL);
        if(message.exec() != QMessageBox::Yes)
        {
            return;
        }
    }
    if(sql.contains("delete")&&!sql.contains("where")){
        QMessageBox::critical(this, "错误","操作不被允许");
        return;
    }
    QSqlQuery query(db);
    if(!query.exec(sql)){
        QSqlError sqlerror= query.lastError();
        qDebug()<<sqlerror.nativeErrorCode();
        QString errortext=sqlerror.text();
        qDebug()<<errortext;
        if(errortext==""){
            errortext="empty query";
        }
        QMessageBox::critical(this, "错误", errortext);
    }
    QList<Item*> items;
    items.clear();
    while (query.next()) {
        items.push_back(new Item(query.value("date").toString(),query.value("href").toString(),query.value("description").toString(),query.value("type").toString()));
    }
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(items.length());
    int i=0;
    for(Item* item : items){
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(item->date));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(item->href));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(item->description));
        ui->tableWidget->setItem(i,3,new QTableWidgetItem(item->type));
        i++;
    }
}
