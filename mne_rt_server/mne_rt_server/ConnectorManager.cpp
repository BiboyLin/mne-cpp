//=============================================================================================================
/**
* @file     connectormanager.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     implementation of the ConnectorManager Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectormanager.h"
#include "mne_rt_server.h"
#include "commandserver.h"
#include "fiffstreamserver.h"

#include "IConnector.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectorManager::ConnectorManager(FiffStreamServer* p_pFiffStreamServer, QObject *parent)
: QPluginLoader(parent)
, m_pFiffStreamServer(p_pFiffStreamServer)
{
    init();
}


//*************************************************************************************************************

ConnectorManager::~ConnectorManager()
{
    QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
    for( ; it != s_vecConnectors.end(); ++it)
        delete (*it);
}


//*************************************************************************************************************

void ConnectorManager::clearConnectorActivation()
{
    // deactivate activated connectors
    if(s_vecConnectors.size() > 0)
    {
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
            if((*it)->isActive())
                (*it)->setStatus(false);
    }
}


//*************************************************************************************************************

void ConnectorManager::comConlist(Command p_command)
{
    //ToDO JSON
    m_commandManager["conlist"].reply(this->getConnectorList());
}


//*************************************************************************************************************

void ConnectorManager::comSelcon(Command p_command)
{
    bool t_isInt;
    qint32 t_id = p_command.pValues()[0].toInt();
    if(t_isInt)
        m_commandManager["selcon"].reply(this->setActiveConnector(t_id));
}


//*************************************************************************************************************

void ConnectorManager::comStart(Command p_command)//comMeas
{
    emit startMeasConnector();
    m_commandManager["start"].reply("Starting active connector.\n");
}


//*************************************************************************************************************

void ConnectorManager::comStopAll(Command p_command)
{
    emit stopMeasConnector();
    m_commandManager["stop-all"].reply("Stoping all connectors.\r\n");
}


//*************************************************************************************************************

void ConnectorManager::connectActiveConnector()
{
    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

    if(t_activeConnector)
    {
        // use signal slots instead of call backs
        //Consulting the Signal/Slot documentation describes why the Signal/Slot approach is better:
        //    Callbacks have two fundamental flaws: Firstly, they are not type-safe. We can never be certain
        //    that the processing function will call the callback with the correct arguments.
        //    Secondly, the callback is strongly coupled to the processing function since the processing
        //    function must know which callback to call.
        //Do be aware of the following though:
        //    Compared to callbacks, signals and slots are slightly slower because of the increased
        //    flexibility they provide
        //The speed probably doesn't matter for most cases, but there may be some extreme cases of repeated
        //calling that makes a difference.

        //
        // Meas Info
        //
        // connect command server and connector manager
        QObject::connect(   this->m_pFiffStreamServer, &FiffStreamServer::requestMeasInfo,
                            t_activeConnector, &IConnector::requestMeasInfo);

        // connect connector manager and fiff stream server
        QObject::connect(   t_activeConnector, &IConnector::remitMeasInfo,
                            this->m_pFiffStreamServer, &FiffStreamServer::forwardMeasInfo);

        //
        // Raw Data
        //
        // connect command server and connector manager
        QObject::connect(   this, &ConnectorManager::startMeasConnector,
                            t_activeConnector, &IConnector::requestMeas);
        // connect connector manager and fiff stream server
        QObject::connect(   t_activeConnector, &IConnector::remitRawBuffer,
                            this->m_pFiffStreamServer, &FiffStreamServer::forwardRawBuffer);
        // connect command server and connector manager
        QObject::connect(   this, &ConnectorManager::stopMeasConnector,
                            t_activeConnector, &IConnector::requestMeasStop);

        //
        // Reset Raw Buffer Size
        //
        QObject::connect(   this, &ConnectorManager::setBufferSize,
                            t_activeConnector, &IConnector::requestSetBufferSize);
    }
    else
    {
        printf("Error: Can't connect, no connector active!\n");
    }
}


//*************************************************************************************************************

void ConnectorManager::disconnectActiveConnector()
{
    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

    if(t_activeConnector)
    {
        // use signal slots instead of call backs
        //Consulting the Signal/Slot documentation describes why the Signal/Slot approach is better:
        //    Callbacks have two fundamental flaws: Firstly, they are not type-safe. We can never be certain
        //    that the processing function will call the callback with the correct arguments.
        //    Secondly, the callback is strongly coupled to the processing function since the processing
        //    function must know which callback to call.
        //Do be aware of the following though:
        //    Compared to callbacks, signals and slots are slightly slower because of the increased
        //    flexibility they provide
        //The speed probably doesn't matter for most cases, but there may be some extreme cases of repeated
        //calling that makes a difference.

        //
        // Meas Info
        //
        this->disconnect(t_activeConnector);
        //
        t_activeConnector->disconnect(this->m_pFiffStreamServer);

        this->m_pFiffStreamServer->disconnect(t_activeConnector);

        //
        // Raw Data
        //
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);
        //
//        t_activeConnector->disconnect(t_pMNERTServer->m_pFiffStreamServer);
        // connect command server and connector manager
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);

        //
        // Reset Raw Buffer
        //
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);
    }
    else
    {
        printf("Error: Can't connect, no connector active!\n");
    }
}


//*************************************************************************************************************

IConnector* ConnectorManager::getActiveConnector()
{
    QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
    for( ; it != s_vecConnectors.end(); ++it)
    {
        if((*it)->isActive())
            return *it;
    }

    return NULL;
}


//*************************************************************************************************************

QByteArray ConnectorManager::getConnectorList() const
{
    QByteArray t_blockConnectorList;
    if(s_vecConnectors.size() > 0)
    {
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
        {
            if((*it)->isActive())
                t_blockConnectorList.append(QString("  *  (%1) %2\r\n").arg((*it)->getConnectorID()).arg((*it)->getName()));
            else
                t_blockConnectorList.append(QString("     (%1) %2\r\n").arg((*it)->getConnectorID()).arg((*it)->getName()));
        }
    }
    else
        t_blockConnectorList.append(" - no connector loaded - \r\n");

    t_blockConnectorList.append("\r\n");
    return t_blockConnectorList;
}

//*************************************************************************************************************

void ConnectorManager::init()
{
    //insert commands
    QString t_sJsonCommand =
                    "{"
                    "   \"commands\": {"
                    "       \"conlist\": {"
                    "           \"description\": \"Prints and sends all available connectors.\","
                    "           \"parameters\": {}"
                    "        },"
                    "       \"selcon\": {"
                    "           \"description\": \"Selects a new connector, if a measurement is running it will be stopped.\","
                    "           \"parameters\": {"
                    "               \"ConID\": {"
                    "                   \"description\": \"Connector ID\","
                    "                   \"type\": \"int\" "
                    "               }"
                    "           }"
                    "        },"
                    "       \"start\": {"
                    "           \"description\": \"Adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already started, it is triggered.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "        },"
                    "       \"stop-all\": {"
                    "           \"description\": \"Stops the whole acquisition process.\","
                    "           \"parameters\": {}"
                    "        }"
                    "    }"
                    "}";


    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(t_sJsonCommand.toLatin1());
    m_commandManager.insert(t_jsonDocumentOrigin);

    //Connect slots
    m_commandManager.connectSlot(QString("conlist"), this, &ConnectorManager::comConlist);
    m_commandManager.connectSlot(QString("selcon"), this, &ConnectorManager::comSelcon);
    m_commandManager.connectSlot(QString("start"), this, &ConnectorManager::comStart);
    m_commandManager.connectSlot(QString("stop-all"), this, &ConnectorManager::comStopAll);
}


//*************************************************************************************************************

void ConnectorManager::loadConnectors(const QString& dir)
{
    clearConnectorActivation();

    QDir ConnectorsDir(dir);

    printf("Loading connectors in directory... %s\n", ConnectorsDir.path().toUtf8().constData() );

    foreach(QString fileName, ConnectorsDir.entryList(QDir::Files))
    {
        if(fileName.compare("README") == 0 || fileName.compare("plugin.cfg") == 0)
            continue;

        this->setFileName(ConnectorsDir.absoluteFilePath(fileName));
        QObject *pConnector = this->instance();

        printf("\tLoading %s... ", fileName.toUtf8().constData() );

        // IModule
        if(pConnector)
        {
            IConnector* t_pIConnector = qobject_cast<IConnector*>(pConnector);
            t_pIConnector->setStatus(false);

            //Add the curent plugin meta data
            t_pIConnector->setMetaData(this->metaData());

            qDebug() << this->metaData();

            s_vecConnectors.push_back(t_pIConnector);
            printf("[done]\n");
        }
        else
            printf("failed!\n");
    }

    //
    // search config for default connector
    //
    qint32 configConnector = -1;
    QString configFileName("plugin.cfg");
    QFile configFile(dir+"/"+configFileName);
    if(!configFile.open(QIODevice::ReadOnly)) {
        printf("Not able to read config file... %s\n", configFile.fileName().toUtf8().constData());
    }
    else
    {
        printf("\tReading %s... ", configFileName.toUtf8().constData());

        QTextStream in(&configFile);
        QString line = in.readLine();
        QStringList list;
        while (!line.isNull()) {
            list = line.split(":");

            if(list[0].simplified().compare("defaultConnector") == 0)
            {
                configConnector = list[1].simplified().toInt();
                break;
            }
            line = in.readLine();
        }
    }
    if(s_vecConnectors.size() > 0)
    {

        bool activated = false;

        if( configConnector != -1)
        {
            for(qint32 i = 0; i < s_vecConnectors.size(); ++i)
            {
                if(s_vecConnectors[i]->getConnectorID() == configConnector)
                {
                    s_vecConnectors[i]->setStatus(true);
                    printf("activate %s... ", s_vecConnectors[i]->getName());
                    activated = true;
                    break;
                }
            }
        }
        printf("[done]\n");

        //default
        if(!activated)
            s_vecConnectors[0]->setStatus(true);
    }

    //print
    printf("Connector list\n");
    printf(getConnectorList().data());
}


//*************************************************************************************************************

QByteArray ConnectorManager::setActiveConnector(qint32 ID)
{
    QByteArray p_blockClientList;
    QString str;

    if(ID != getActiveConnector()->getConnectorID())
    {
        IConnector* t_pNewActiveConnector = NULL;
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
            if((*it)->getConnectorID() == ID)
                t_pNewActiveConnector = *it;

        if (t_pNewActiveConnector)
        {

           IConnector* t_pActiveConnector = getActiveConnector();

           //Stop and disconnect active connector
           t_pActiveConnector->stop();
           this->disconnectActiveConnector();
           t_pActiveConnector->setStatus(false);

           //set new active connector
           t_pNewActiveConnector->setStatus(true);
           this->connectActiveConnector();

            str = QString("\t%1 activated. ToDo...\r\n\n").arg(t_pNewActiveConnector->getName());
            p_blockClientList.append(str);
        }
        else
        {
            str = QString("\tID %1 doesn't match a connector ID.\r\n\n").arg(ID);
            p_blockClientList.append(str);
            p_blockClientList.append(getConnectorList());
        }
    }
    else
    {
        str = QString("\t%1 is already active.\r\n\n").arg(getActiveConnector()->getName());
        p_blockClientList.append(str);
    }

    return p_blockClientList;
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IConnector*>    ConnectorManager::  s_vecConnectors;
