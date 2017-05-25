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

#ifndef DFNODE_H_INCLUDED
#define DFNODE_H_INCLUDED

#include "v_repLib.h"
#include "DFData.h"
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <QMutex>

class DFNode;
class DFData;
struct DFNodeInlet;
struct DFNodeOutlet;
struct DFConnection;

struct DFNodeIOlet
{
    DFNode *node;
    size_t index;

    bool operator<(const DFNodeIOlet &o) const;
};

struct DFNodeInlet : public DFNodeIOlet
{
};

struct DFNodeOutlet : public DFNodeIOlet
{
};

struct DFConnection
{
    DFNode *src;
    size_t srcOutlet;
    DFNode *dst;
    size_t dstInlet;

    bool operator<(const DFConnection &o) const;
};

class DFException : public std::runtime_error
{
public:
    DFException(DFNode *node, std::string message);
    virtual ~DFException() throw();
    virtual const char * what() const throw();

    DFNode * node() const;

private:
    DFNode *node_;
    std::string message_;
};

typedef size_t DFNodeID;
typedef std::map<DFNodeID, DFNode*> DFNodeIDMap;

class DFNode
{
private:
    static QMutex mutex;
    static DFNodeIDMap byId_;
    static DFNodeID nextNodeId_;
    DFNodeID id_;
    int x_;
    int y_;
    std::vector<std::string> args_;
    std::string text_;
    std::vector<DFNodeInlet> inlets_;
    std::vector<DFNodeOutlet> outlets_;
    std::vector< std::set<DFNodeOutlet> > inboundConnections_;
    std::vector< std::set<DFNodeInlet> > outboundConnections_;

    template<typename T>
    void setNumIOlets(std::vector<T> &v, size_t n)
    {
        size_t oldSize = v.size();
        v.resize(n);
        for(size_t i = oldSize; i < n; i++)
        {
            v[i].node = this;
            v[i].index = i;
        }
    }

    template<typename T>
    void validateIOlet(const std::vector<T> &v, size_t i, const char *n) const
    {
        if(i >= v.size())
            throw std::range_error((boost::format("invalid %s index: %d") % n % i).str());
    }

    void validateInlet(size_t i) const;
    void validateOutlet(size_t i) const;
    void validateNode(DFNode *node) const;

public:
    DFNode(const std::vector<std::string> &args);
    virtual ~DFNode();
    DFNodeID id() const;
    int x() const;
    int y() const;
    void setPos(int x, int y);
    std::string str() const;
    DFNodeInlet inlet(size_t i) const;
    size_t inletCount() const;
    DFNodeOutlet outlet(size_t i) const;
    size_t outletCount() const;
    std::string arg(size_t i);
    size_t argCount();
    std::set<DFNodeOutlet> inboundConnections(size_t inlet) const;
    std::set<DFNodeInlet> outboundConnections(size_t outlet) const;
    std::set<DFConnection> connections(bool inbound = true, bool outbound = true);
    static std::set<DFConnection> allConnections();
    static size_t allConnections(std::vector<int> &srcNodeIds, std::vector<int> &srcOutlets, std::vector<int> &dstNodeIds, std::vector<int> &dstInlets);
    bool isConnected(size_t outlet, DFNode *node, size_t inlet) const;
    void connect(size_t outlet, DFNode *node, size_t inlet);
    void disconnect(size_t outlet, DFNode *node, size_t inlet);
    void disconnectInlet(size_t inlet);
    void disconnectOutlet(size_t outlet);
    void disconnect();
    static DFNode * byId(DFNodeID id);
    static DFNode * byId(DFNodeID id, DFNode *defaultIfNotFound);
    static void deleteById(DFNodeID id);
    static void connect(DFNodeID srcNodeId, size_t srcOutlet, DFNodeID dstNodeId, size_t dstInlet);
    static void disconnect(DFNodeID srcNodeId, size_t srcOutlet, DFNodeID dstNodeId, size_t dstInlet);
    static std::vector<DFNodeID> nodeIds();
    static std::vector<DFNode*> nodes();
    static void getInfo(DFNodeID id, std::string &cmd, int &inletCount, int &outletCount, int &x, int &y);
    virtual void tick();
    static void tickAll();
    static void clearGraph();
    static void loadGraph(std::string filename);
    static void saveGraph(std::string filename);

    virtual simInt getObjectHandle(const std::string &arg);

protected:
    void setNumInlets(size_t n);
    void setNumOutlets(size_t n);
    virtual void onDataReceived(size_t inlet, DFData *data);
    void sendData(size_t outlet, DFData *data);
};

#endif // DFNODE_H_INCLUDED
