// Copyright 2016 Coppelia Robotics GmbH. All rights reserved. 
// marc@coppeliarobotics.com
// www.coppeliarobotics.com
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -------------------------------------------------------------------
// Authors:
// Federico Ferri <federico.ferri.it at gmail dot com>
// -------------------------------------------------------------------

#include "DFEventsListener.h"
#include "DFWindow.h"
#include "DFNode.h"
#include "DFNodeFactory.h"
#include "UIProxy.h"
#include "stubs.h"
#include "debug.h"

extern DFWindow *mainWindow;
extern UIProxy *uiProxy;

DFEventsListener::DFEventsListener()
{
    QDataflowModel *model = mainWindow->canvas->model();
    QObject::connect(model, &QDataflowModel::nodeAdded, this, &DFEventsListener::onNodeAdded);
    QObject::connect(model, &QDataflowModel::nodeRemoved, this, &DFEventsListener::onNodeRemoved);
    QObject::connect(model, &QDataflowModel::nodePosChanged, this, &DFEventsListener::onNodePosChanged);
    QObject::connect(model, &QDataflowModel::nodeTextChanged, this, &DFEventsListener::onNodeTextChanged);
    QObject::connect(model, &QDataflowModel::nodeInletCountChanged, this, &DFEventsListener::onNodeInletCountChanged);
    QObject::connect(model, &QDataflowModel::nodeOutletCountChanged, this, &DFEventsListener::onNodeOutletCountChanged);
    QObject::connect(model, &QDataflowModel::connectionAdded, this, &DFEventsListener::onConnectionAdded);
    QObject::connect(model, &QDataflowModel::connectionRemoved, this, &DFEventsListener::onConnectionRemoved);
    QObject::connect(this, &DFEventsListener::setNodeInfo, uiProxy, &UIProxy::setNodeInfo);
    QObject::connect(this, &DFEventsListener::graphChanged, this, &DFEventsListener::onGraphChanged);
}

void DFEventsListener::onNodeAdded(QDataflowModelNode *node)
{
    DBG << (void*)node << " text='" << node->text().toStdString() << "'" << std::endl;
    if(node->text() == "")
    {
        emit setNodeInfo(node, "", 0, 0, false, true);
        return;
    }
    std::string cmd = node->text().toStdString();
    DFNode *dfnode = nodeFactory.create(cmd, node->pos().x(), node->pos().y());
    node->setProperty("DFNode", qVariantFromValue((void*)dfnode));
    DBG << "created DFNode " << dfnode->id() << std::endl;
    emit graphChanged();
}

void DFEventsListener::onNodeRemoved(QDataflowModelNode *node)
{
    DBG << "node=" << (void*)node << std::endl;
    if(!node->property("DFNode").isValid()) return;
    DFNode *dfnode = (DFNode*)node->property("DFNode").value<void*>();
    node->setProperty("DFNode", QVariant());
    DBG << "removing DFNode " << dfnode->id() << std::endl;
    delete dfnode;
    emit graphChanged();
}

void DFEventsListener::onNodePosChanged(QDataflowModelNode *node, QPoint pos)
{
    DBG << "node=" << (void*)node << std::endl;
    if(node->property("DFNode").isValid())
    {
        DFNode *dfnode = (DFNode*)node->property("DFNode").value<void*>();
        dfnode->setPos(node->pos().x(), node->pos().y());
        emit graphChanged();
    }
}

void DFEventsListener::onNodeTextChanged(QDataflowModelNode *node, QString text)
{
    DBG << "node=" << (void*)node << " text='" << node->text().toStdString() << "'" << std::endl;

    // backup connections:
    DFNode *oldnode = 0;
    std::set<DFConnection> oldconns;
    if(node->property("DFNode").isValid())
    {
        DFNode *dfnode = (DFNode*)node->property("DFNode").value<void*>();
        if(dfnode->str() == node->text().toStdString()) return;
        oldnode = dfnode;
        oldconns = oldnode->connections();
        node->setProperty("DFNode", QVariant());
        DBG << "removing DFNode " << dfnode->id() << std::endl;
        delete dfnode;
    }

    // ignore null node:
    if(node->text() == "")
    {
        emit setNodeInfo(node, "", 0, 0, false, true);
        emit graphChanged();
        return;
    }

    std::string cmd = node->text().toStdString();
    DFNode *dfnode = 0L;
    try
    {
        dfnode = nodeFactory.create(cmd, node->pos().x(), node->pos().y());
    }
    catch(std::runtime_error &ex)
    {
        simAddStatusbarMessage((boost::format("Dataflow: object creation error: %s") % ex.what()).str().c_str());
        emit setNodeInfo(node, node->text().toStdString(), 0, 0, false, true);
        emit graphChanged();
        return;
    }
    DBG << "created DFNode " << dfnode->id() << std::endl;
    node->setProperty("DFNode", qVariantFromValue((void*)dfnode));
    emit setNodeInfo(node, dfnode->str(), dfnode->inletCount(), dfnode->outletCount(), true, false);
    emit graphChanged();

    // restore connections:
    BOOST_FOREACH(DFConnection c, oldconns)
    {
        DFNode *s = c.src == oldnode ? dfnode : c.src;
        DFNode *d = c.dst == oldnode ? dfnode : c.dst;
        if(s->outletCount() <= c.srcOutlet) continue;
        if(d->inletCount() <= c.dstInlet) continue;
        s->connect(c.srcOutlet, d, c.dstInlet);
    }
}

void DFEventsListener::onNodeInletCountChanged(QDataflowModelNode *node, int count)
{
    DBG << "node=" << (void*)node << std::endl;
}

void DFEventsListener::onNodeOutletCountChanged(QDataflowModelNode *node, int count)
{
    DBG << "node=" << (void*)node << std::endl;
}

void DFEventsListener::onConnectionAdded(QDataflowModelConnection *conn)
{
    DBG << "conn=" << (void*)conn << std::endl;
    QVariant vsrc = conn->source()->node()->property("DFNode"),
             vdst = conn->dest()->node()->property("DFNode");
    if(!vsrc.isValid() || !vdst.isValid()) return;
    DFNode *src = (DFNode*)vsrc.value<void*>();
    DFNode *dst = (DFNode*)vdst.value<void*>();
    src->connect(conn->source()->index(), dst, conn->dest()->index());
    emit graphChanged();
}

void DFEventsListener::onConnectionRemoved(QDataflowModelConnection *conn)
{
    DBG << "conn=" << (void*)conn << std::endl;
    QVariant vsrc = conn->source()->node()->property("DFNode"),
             vdst = conn->dest()->node()->property("DFNode");
    if(!vsrc.isValid() || !vdst.isValid()) return;
    DFNode *src = (DFNode*)vsrc.value<void*>();
    DFNode *dst = (DFNode*)vdst.value<void*>();
    src->disconnect(conn->source()->index(), dst, conn->dest()->index());
    emit graphChanged();
}

void DFEventsListener::onGraphChanged()
{
    DBG << "new graph:" << std::endl;
    DFNode::saveGraph("");
}

