#ifndef _NORMALIZE_HPP
#define _NORMALIZE_HPP

#include "dynarray.h"

#include "pointeroffset.hpp"

#include "container.hpp"
#include "byref.hpp"
#include "genio.h"
#include "threadlocal.hpp"
#include "funcs.hpp"
#include <algorithm>
#include "debugprint.hpp"

//FIXME: leave rules that don't occur in normalization groups alone (use some original/default value)
template <class W>
struct NormalizeGroups {
    typedef W weight_type;
    typedef PointerOffset<W> value_type; // pointer offsets
    typedef Array<value_type> Group;
    typedef SwapBatch<Group> Groups;
    struct max_accum {
        T max;

        max_accum() : max() {}
        template <class T>
        void operator()(const T& t) {
            if (max < t)
                max = t;
        }
        operator T &() { return max; }
    };
    max_accum<value_type> max_offset;

    NormalizeGroups(std::string basename,unsigned batchsize,value_type watch_value)  : norm_groups(basename,batchsize)
    {
        if (watch_value.get_offset())

    }

    Groups norm_groups;


    value_type max_offset;
    unsigned num_groups() const {
        return norm_groups.size();
    }
    unsigned num_params() const {
        unsigned sum=0;
        for (typename Groups::const_iterator i=norm_groups.begin(),e=norm_groups.end();i!=e;++i)
            sum+=i->size();
        return sum;
    }
    Group *find_group_holding(value_type v) {
        typename Groups::iterator i=norm_groups.begin(),e=norm_groups.end();
        DBPC3("find group",v,norm_groups);
        for (;i!=e;++i) {
            DBP_ADD_VERBOSE(2);
            DBP2(i,v);
            typename Group::iterator e2=i->end();
            if (std::find(i->begin(),e2,v) != e2)
                return &(*i);
        }
        return NULL;
    }

    size_t max_index() const {
        return max_offset.get_index();
    }
    size_t required_size() const {
        return max_index()+1;
    }

    W *base;
    W *dest;
    W maxdiff;
    std::ostream *log;
    enum { ZERO_ZEROCOUNTS=0,SKIP_ZEROCOUNTS=1,UNIFORM_ZEROCOUNTS=2};
    int zerocounts; // use enum vals
    unsigned maxdiff_index;
    typedef typename Group::iterator GIt;
    void print_stats(std::ostream &out=std::cerr) const {
        unsigned npar=num_params();
        unsigned ng=num_groups();
        out << ng << " normalization groups, "  << npar<<" parameters, "<<(float)npar/ng<<" average parameters/group";
    }
    void operator ()(Group &i) {
        GIt end=i.end(), beg=i.begin();
        weight_type sum=0;
        for (GIt j=beg;j!=end;++j) {
            weight_type &w=*(j->add_base(base));
            sum+=w;
        }
#define DODIFF(d,w) do {weight_type diff = absdiff(d,w);if (maxdiff<diff) {maxdiff_index=j->get_index();DBP5(d,w,maxdiff,diff,maxdiff_index);maxdiff=diff;} } while(0)
        if (sum > 0) {
            DBPC2("Normalization group with",sum);
            for (GIt j=beg;j!=end;++j) {
                weight_type &w=*(j->add_base(base));
                weight_type &d=*(j->add_base(dest));
                DBP4(j->get_index(),d,w,w/sum);
                weight_type prev=d;
                d=w/sum;
                DODIFF(d,prev);
            }
        } else {
            if(log)
                *log << "Zero counts for normalization group #" << 1+(&i-norm_groups.begin())  << " with first parameter " << beg->get_index() << " (one of " << i.size() << " parameters)";
            if (zerocounts!=SKIP_ZEROCOUNTS) {
                weight_type setto;
                if (zerocounts == UNIFORM_ZEROCOUNTS) {
                    setto=1. / (end-beg);
                    if(log)
                        *log << " - setting to uniform probability " << setto << std::endl;
                } else {
                    setto=0;
                    if(log)
                        *log << " - setting to zero probability." << std::endl;
                }
                for (GIt j=beg;j!=end;++j) {
                    weight_type &w=*(j->add_base(base));
                    weight_type &d=*(j->add_base(dest));
                    DODIFF(d,setto);
                    d=setto;
                }
            }
        }
#undef DO_DIFF
    }
#ifdef NORMALIZE_SEEN
    void copy_unseen(W *src, W *to) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                to[i]=src[i];
        }
    }
    void set_unseen_to(W src, W *to) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                to[i]=src;
        }
    }
    template <class F>
    void enumerate_seen(F f) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (seen_index[i])
                f(i);
        }
    }
    template <class F>
    void enumerate_unseen(F f) {
        for (unsigned i=0,e=seen_index.size();i!=e;++i) {
            if (!seen_index[i])
                f(i);
        }
    }
#endif
    template <class T>
    void visit(Group &group, T tag) {
        GIt beg=group.begin(),end=group.end();
        W sum=0;
        for (GIt i=beg;i!=end;++i) {
            W &w=*(i->add_base(base));
            tag(w);
            sum += w;
        }
        if (sum > 0)
            for (GIt i=beg;i!=end;++i) {
                W &w=*(i->add_base(base));
                w /= sum;
            }
    }
    template <class T>
    void init(W *w,T tag) {
        base=w;
        enumerate(norm_groups,*this,tag);
    }
    void init_uniform(W *w) {
        init(w,set_one());
    }
    void init_random(W *w) {
        base=w;
        init(w,set_random_pos_fraction());
    }

// array must have values for all max_index()+1 rules
// returns maximum change
    void normalize(W *array_base) {
        normalize(array_base,array_base);
    }
    void normalize(W *array_base, W* _dest, int _zerocounts=UNIFORM_ZEROCOUNTS, ostream *_log=NULL) {
#ifdef 0
        // don't need: no longer global
        SetLocal<W*> g1(base,array_base);
       SetLocal<W*> g2(dest,_dest);
       SetLocal<W>g3(maxdiff,W(0));
#else
        base=array_base;
        dest=_dest;
        maxdiff.setZero();
#endif
//        DBP(maxdiff);
        DBP_INC_VERBOSE;
#ifdef DEBUG
        unsigned size=required_size();
#endif
        DBPC2("Before normalize from base->dest",Array<W>(base,base+size));

        zerocounts=_zerocounts;
        log=_log;
        enumerate(norm_groups,ref(*this));
        DBPC2("After normalize:",Array<W>(dest,dest+size));
    }
};

template <class charT, class Traits,class C>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, NormalizeGroups<C> &arg)
{
    arg.norm_groups.read_all_enumerate(is,ref(arg.max_offset));
    return is;
}


template <class charT, class Traits,class C>
std::basic_ostream<charT,Traits>&
operator <<
    (std::basic_ostream<charT,Traits>& os, const NormalizeGroups<C> &arg)
{
    return os << arg.norm_groups;
}

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_NORMALIZE )
{
    typedef Weight W;
    FixedArray<W> w(4u);
    w[0]=1;
    w[1]=2;
    w[2]=3;
    w[3]=4;
    NormalizeGroups<W> ng("tmp.test.normalize",100);
    string s="((1) (2 3))";
    istringstream is(s);
    BOOST_CHECK(is >> ng);
    BOOST_CHECK(ng.max_index() == 3);
//    cerr << Weight::out_always_real;
//    cout << Weight::out_variable;
//    DBP(w);
//    DBP(ng);
    ng.normalize(w.begin());
//    BOOST_CHECK_CLOSE(w[2].getReal()+w[3].getReal(),1,1e-6);
//    BOOST_CHECK_CLOSE(w[2].getReal()*4,w[3].getReal()*3,1e-6);

//    DBP(w);
}
#endif

#endif
