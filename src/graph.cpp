#include <queue>

#include "graph.h"

GCTS_BEGIN

Graph::Graph(std::set<Vertex *> allVertices) noexcept
    : vtces_(std::move(allVertices))
{
    for(auto v : vtces_)
    {
        if(v->isSrc)
            srcs_.insert(v);
    }
}

Graph::MinCutResult Graph::findMinCut()
{
    // find max flow

    for(auto v : vtces_)
    {
        for(auto e : v->edges)
        {
            if(e)
                e->a2bFlow = 0;
        }
    }

    while(runMaxFlowIteration())
        ; // do nothing

    // run bfs on res graph

    for(auto v : vtces_)
        v->bfsAccesser = nullptr;

    std::queue<Vertex *> vtxQueue;
    for(auto v : srcs_)
        vtxQueue.push(v);

    while(!vtxQueue.empty())
    {
        auto v = vtxQueue.front();
        vtxQueue.pop();

        for(auto e : v->edges)
        {
            if(!e)
                continue;

            if(v == e->a)
            {
                if(!e->b->isSrc && !e->b->bfsAccesser && e->a2bRes() > 0)
                {
                    e->b->bfsAccesser = e;
                    vtxQueue.push(e->b);
                }
            }
            else
            {
                if(!e->a->isSrc && !e->a->bfsAccesser && e->b2aRes() > 0)
                {
                    e->a->bfsAccesser = e;
                    vtxQueue.push(e->a);
                }
            }
        }
    }

    // find reachable boundary

    MinCutResult ret;

    const auto isReachable = [](const Vertex *v)
    {
        return v->isSrc || v->bfsAccesser;
    };

    for(auto v : vtces_)
    {
        if(!isReachable(v))
            continue;

        ret.reachableVertices.insert(v);

        for(auto e : v->edges)
        {
            if(e && !isReachable(e->another(v)))
                ret.cut.insert(e);
        }
    }

    return ret;
}

int Graph::runMaxFlowIteration()
{
    // run bfs to find a res path

    for(auto v : vtces_)
        v->bfsAccesser = nullptr;

    std::queue<Vertex *> vtxQueue;
    for(auto v : srcs_)
        vtxQueue.push(v);

    Vertex *v = nullptr;
    while(!vtxQueue.empty())
    {
        v = vtxQueue.front();
        vtxQueue.pop();

        if(v->isSink)
            break;

        for(auto e : v->edges)
        {
            if(!e)
                continue;

            if(v == e->a)
            {
                if(!e->b->isSrc && !e->b->bfsAccesser && e->a2bRes() > 0)
                {
                    e->b->bfsAccesser = e;
                    vtxQueue.push(e->b);
                }
            }
            else
            {
                if(!e->a->isSrc && !e->a->bfsAccesser && e->b2aRes() > 0)
                {
                    e->a->bfsAccesser = e;
                    vtxQueue.push(e->a);
                }
            }
        }

        v = nullptr;
    }

    if(!v)
        return 0;

    // find capacity of the path

    int pathRes = std::numeric_limits<int>::max();
    for(auto tv = v; !tv->isSrc;)
    {
        Edge *e = tv->bfsAccesser;
        if(tv == e->a)
        {
            pathRes = std::min(pathRes, e->b2aRes());
            tv = e->b;
        }
        else
        {
            pathRes = std::min(pathRes, e->a2bRes());
            tv = e->a;
        }
    }

    // update edge flow

    for(auto tv = v; !tv->isSrc;)
    {
        Edge *e = tv->bfsAccesser;
        if(tv == e->a)
        {
            e->a2bFlow -= pathRes;
            tv = e->b;
        }
        else
        {
            e->a2bFlow += pathRes;
            tv = e->a;
        }
    }

    return pathRes;
}

GCTS_END
