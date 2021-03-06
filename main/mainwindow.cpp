#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("ARP ATTACK @phoexina");


    ipAdapterinfo=getAllAdapterInfo();
    initBox();
    macmodel = new QStandardItemModel();
    fakemodel = new QStandardItemModel();
    ui->ipmacTable->setModel(macmodel);
    ui->ipmacTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->ipmacTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ipmacTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ipmacTable->horizontalHeader()->setHighlightSections(false);
    ui->ipmacTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    fakemodel = new QStandardItemModel();
    ui->fakeTable->setModel(fakemodel);
    ui->fakeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->fakeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fakeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fakeTable->horizontalHeader()->setHighlightSections(false);
    ui->fakeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);


    initList();
    scanflag=false;
    connect(ui->actionARP_SCAN,SIGNAL(triggered(bool)),this,SLOT(showScanHelp()));
    connect(ui->actionARP_ATTACK,SIGNAL(triggered(bool)),this,SLOT(showAttackHelp()));
    connect(ui->actionPhoexina,SIGNAL(triggered(bool)),this,SLOT(showPhoexina()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initBox(){
    QString str;
    PIP_ADAPTER_INFO cur=ipAdapterinfo;
    while(cur){
        IP_ADDR_STRING *pIpAddrString =&(cur->IpAddressList);
        str=QLatin1String(cur->Description)+'\n';
        str+=QLatin1String(pIpAddrString->IpAddress.String)+" | ";
        str+=QLatin1String(pIpAddrString->IpMask.String);
        ui->adapterBox->addItem(str);
        cur=cur->Next;
   }

}

void MainWindow::initList(){
    macmodel->setHeaderData(0,Qt::Horizontal,"IP");
    macmodel->setHeaderData(1,Qt::Horizontal,"MAC");

    ui->ipmacTable->setColumnWidth(0,370);
    ui->ipmacTable->setColumnWidth(1,370);
}

void MainWindow::initFakeList(){
    fakemodel->setHeaderData(0,Qt::Horizontal,"Fake host");
    fakemodel->setHeaderData(1,Qt::Horizontal,"Dst host");

    ui->fakeTable->setColumnWidth(0,550);
    ui->fakeTable->setColumnWidth(1,210);
}




void MainWindow::on_scanButton_clicked()
{
    if(scanflag){
        ui->scanButton->setText("SCAN");
        ui->adapterBox->setEnabled(true);
        scanflag=false;


    }else{
        macmodel->clear();
        ui->scanButton->setText("STOP");
        ui->adapterBox->setEnabled(false);
        scanflag=true;

        int code=ui->adapterBox->currentIndex();
        PIP_ADAPTER_INFO cur =ipAdapterinfo;
        while(code--){cur=cur->Next;}
        ULONG ulIpMask = ntohl(::inet_addr(cur->IpAddressList.IpMask.String));
        ULONG ulIpAddress = ntohl(::inet_addr(cur->IpAddressList.IpAddress.String));
        ULONG ulNetName = ulIpAddress & ulIpMask;
        UINT nNumofHost = (~ulIpMask) - 2;

        UINT uip;
        for(int i=1;i<=nNumofHost;i++){
            if(!scanflag) break;
            uip=ulNetName + i;
            ScanThread* th=new ScanThread(this);
            connect(th, &ScanThread::send, this, &MainWindow::show_list);
            connect(th, &ScanThread::log, this, &MainWindow::show_log);
            th->setInfo(ulIpAddress,uip);
            th->start();
            QApplication::processEvents();
            Sleep(100);
        }
        ui->scanButton->setText("SCAN");
        ui->adapterBox->setEnabled(true);
        scanflag=false;

    }


}


void MainWindow::show_list(QString ip,QString mac){
    int row=macmodel->rowCount();
    macmodel->setItem(row, 0, new QStandardItem(ip));
    macmodel->setItem(row, 1, new QStandardItem(mac));
    initList();

}

void MainWindow::show_log(QString s){
    ui->logBrowser->append(s);
    ui->logBrowser->append("-----------------------------------");
}



void MainWindow::on_adapterBox_activated(const QString &arg1){
    macmodel->clear();
}


void MainWindow::on_ipmacTable_doubleClicked(const QModelIndex &index){
    QMessageBox msgBox;
    msgBox.setText("ARP ATTACK START");
    msgBox.setInformativeText("Are you sure you want to attack this host?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
    if(ret == QMessageBox::Cancel) return;

    int row = index.row();
    if(row==-1){
        show_log("Error: No choose dst host!");
        return;
    }

    QString dstip=macmodel->item(row,0)->text();
    QString dstmac=macmodel->item(row,1)->text();
    QString srcmac=ui->fakemacText->text();
    QString srcip=ui->fakeipText->text();

    if(srcmac=="") srcmac="00-00-00-00-00-00";
    if(srcip=="") srcip="0.0.0.0";

    int code=ui->adapterBox->currentIndex();
    PIP_ADAPTER_INFO cur =ipAdapterinfo;
    while(code--){cur=cur->Next;}

    SendThread* th=new SendThread();
    connect(th, &SendThread::log, this, &MainWindow::show_log);
    sdthList.append(th);

    int frow=fakemodel->rowCount();
    fakemodel->setItem(frow, 0, new QStandardItem(srcip+"|"+srcmac));
    fakemodel->setItem(frow, 1, new QStandardItem(dstip));
    initFakeList();
    QApplication::processEvents();

    char mymac[20];
    sprintf(mymac,"%02x-%02x-%02x-%02x-%02x-%02x",cur->Address[0],cur->Address[1],cur->Address[2],cur->Address[3],cur->Address[4],cur->Address[5]);
    th->setInfo(mymac,cur->AdapterName,srcmac,srcip,dstmac,dstip);
    th->start();

}


void MainWindow::on_fakeTable_doubleClicked(const QModelIndex &index){
    QMessageBox msgBox;
    msgBox.setText("ARP ATTACK STOP");
    msgBox.setInformativeText("Are you sure you want to stop this attack?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
    if(ret == QMessageBox::Cancel) return;

    int row = index.row();
    if(row==-1){
        show_log("Error: No choose attack!");
        return;
    }
    sdthList[row]->stop();
    sdthList.remove(row);
    fakemodel->removeRow(row);


}

void MainWindow::showScanHelp(){
    QMessageBox msgBox;
    msgBox.setText("ARP SCAN TIPS");
    msgBox.setInformativeText("1.??????SCAN????????????????????????????????????STOP?????????????????????\n"
                              "2.??????STOP????????????????????????????????????SCAN?????????????????????\n"
                              "3.????????????????????????????????????????????????????????????2\n"
                              "4.Log?????????????????????????????????\n");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

}

void MainWindow::showAttackHelp(){
    QMessageBox msgBox;
    msgBox.setText("ARP ATTACK TIPS");
    msgBox.setInformativeText("1.?????????????????????????????????ip???mac??????\n"
                              "2.??????????????????????????????????????????????????????????????????\n"
                              "3.??????????????????????????????????????????????????????\n"
                              "4.????????????????????????????????????????????????????????????\n"
                              "5.????????????????????????????????????????????????????????????\n"
                              "6.??????????????????????????????????????????\n"
                              "7.????????????????????????????????????????????????0v0");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

}

void MainWindow::showPhoexina(){
    QMessageBox msgBox;
    msgBox.setText("Hello,I'm Phoexina");
    msgBox.setInformativeText("You will see the loveliest creature in the world??????~");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
    ui->logBrowser->setText(
                      "..................................................\n"
                      "...............@@@@@@@@@@@........................\n"
                      "..........@@@@@@@........@@@@.....................\n"
                      "........@@@....@............@@@...................\n"
                      ".....@@@.........@@..........@@@..................\n"
                      ".....@@..........@@.....@@....@@@@@@..............\n"
                      "...@@@.............................@@@@@..........\n"
                      "..@@@.................................@@@.........\n"
                      "..@@.....................................@@@......\n"
                      "..@@.......................................@@@....\n"
                      ".@@@................@@@.....................@@....\n"
                      ".@@....................@@@.................@@@....\n"
                      ".@@......................@@................@@.....\n"
                      "..@@@..@@................@@.................@@....\n"
                      "...@@@..@@@.................................@@....\n"
                      "....@@@@@.@@@@@...........@@@..............@@.....\n"
                      "........@@@...@@..@@@@..@@@@@...........@@@@......\n"
                      "...........@@@@@@....@......@@.....@.....@@@......\n"
                      ".................@@@@@@..@@@@@@@@@@@@@@@...@@@....\n"
                      "..................................@@@@@@@@@..@@@..\n"
                      ".......................................@@@@@@@@@@@\n"
                      "...........................................@@@@@@@\n"
                      "..................................................\n"
                      "...???????????????????????????????????????????????????Log??????0v0...\n");

}

