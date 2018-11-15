#ifndef cutflowutil_h
#define cutflowutil_h

#include "ttreex.h"
#include "printutil.h"
#include <tuple>
#include <vector>
#include <map>
#include "TH1.h"
#include "TString.h"
#include <iostream>
#include <algorithm>
#include <sys/ioctl.h>
#include <functional>

//#define USE_TTREEX
#define USE_CUTLAMBDA
#define TREEMAPSTRING std::string
#define CUTFLOWMAPSTRING TString
#define DATA c_str

namespace RooUtil
{
    namespace CutflowUtil
    {

        class CutNameList
        {
            public:
            std::vector<TString> cutlist;
            CutNameList() {}
            CutNameList(const CutNameList& cutnamelist) { cutlist = cutnamelist.cutlist; }
            void clear() { cutlist.clear(); }
            void addCutName(TString cutname) { cutlist.push_back(cutname); }
            void print() { for (auto& str : cutlist) std::cout << str << std::endl; }
        };

        class CutNameListMap
        {
            public:
            std::map<TString, CutNameList> cutlists;
            std::vector<TString> cutlist;
            CutNameList& operator[] (TString name) { return cutlists[name]; }
            void clear() { cutlists.clear(); }
            void print() { for (auto& cutlist : cutlists) { std::cout << "CutNameList - " << cutlist.first << std::endl; cutlist.second.print(); } }
            std::map<TString, std::vector<TString>> getStdVersion()
            {
                std::map<TString, std::vector<TString>> obj_cutlists;
                for (auto& cutlist : cutlists)
                    obj_cutlists[cutlist.first] = cutlist.second.cutlist;
                return obj_cutlists;
            }
        };

        void createCutflowBranches(CutNameListMap& cutlists, RooUtil::TTreeX& tx);
        void createCutflowBranches(std::map<TString, std::vector<TString>>& cutlists, RooUtil::TTreeX& tx);
        std::tuple<std::vector<bool>, std::vector<float>> getCutflow(std::vector<TString> cutlist, RooUtil::TTreeX& tx);
        std::pair<bool, float> passCuts(std::vector<TString> cutlist, RooUtil::TTreeX& tx);
        void fillCutflow(std::vector<TString> cutlist, RooUtil::TTreeX& tx, TH1F* h);
        void fillRawCutflow(std::vector<TString> cutlist, RooUtil::TTreeX& tx, TH1F* h);
        std::tuple<std::map<CUTFLOWMAPSTRING, TH1F*>, std::map<CUTFLOWMAPSTRING, TH1F*>> createCutflowHistograms(CutNameListMap& cutlists, TString syst="");
        std::tuple<std::map<CUTFLOWMAPSTRING, TH1F*>, std::map<CUTFLOWMAPSTRING, TH1F*>> createCutflowHistograms(std::map<TString, std::vector<TString>>& cutlists, TString syst="");
        void saveCutflowHistograms(std::map<CUTFLOWMAPSTRING, TH1F*>& cutflows, std::map<CUTFLOWMAPSTRING, TH1F*>& rawcutflows);
//        void fillCutflowHistograms(CutNameListMap& cutlists, RooUtil::TTreeX& tx, std::map<TString, TH1F*>& cutflows, std::map<TString, TH1F*>& rawcutflows);
//        void fillCutflowHistograms(std::map<TString, std::vector<TString>>& cutlists, RooUtil::TTreeX& tx, std::map<TString, TH1F*>& cutflows, std::map<TString, TH1F*>& rawcutflows);

    }

    class CutTree
    {
        public:
            TString name; 
            CutTree* parent;
            std::vector<CutTree*> parents;
            std::vector<CutTree*> children;
            std::map<TString, CutTree*> systs;
            bool pass;
            float weight;
            bool pass_this_cut;
            float weight_this_cut;
            std::function<bool()> pass_this_cut_func;
            std::function<float()> weight_this_cut_func;
//            std::vector<TString> hists1d;
//            std::vector<std::tuple<TString, TString>> hists2d;
            std::map<TString, std::vector<std::tuple<TH1F*, TString>>> hists1d;
            std::map<TString, std::vector<std::tuple<TH2F*, TString, TString>>> hists2d;
            std::vector<std::tuple<int, int, unsigned long long>> eventlist;
            CutTree(TString n) : name(n), parent(0), pass(false), weight(0) {}
            ~CutTree()
            {
                for (auto& child : children)
                {
                    if (child)
                        delete child;
                }
            }
            void printCuts(int indent=0, std::vector<int> multichild=std::vector<int>())
            {
                struct winsize w;
                ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                int colsize = min(w.ws_col - 50, 600);
                if (indent == 0)
                {
                    TString header = "Cut name";
                    int extra = colsize - header.Length();
                    for (int i = 0; i < extra; ++i) header += " ";
                    header += "|pass|weight|systs";
                    print(header);
                    TString delimiter = "";
                    for (int i = 0; i < w.ws_col-10; ++i) delimiter += "=";
                    print(delimiter);
                }
                TString msg = "";
                for (int i = 0; i < indent; ++i)
                {
                    if (std::find(multichild.begin(), multichild.end(), i+1) != multichild.end())
                    {
                        if (indent == i + 1)
                            msg += " +";
                        else
                            msg += " |";
                    }
                    else
                        msg += "  ";
                }
                msg += name;
                int extrapad = colsize - msg.Length() > 0 ? colsize - msg.Length() : 0;
                for (int i = 0; i < extrapad; ++i)
                    msg += " ";
                msg += TString::Format("| %d | %.5f|", pass, weight);
                for (auto& key : systs)
                {
                    msg += key.first + " ";
                }
                print(msg);
                if (children.size() > 1)
                    multichild.push_back(indent+1);
                for (auto& child : children)
                    (*child).printCuts(indent+1,multichild);
            }
            void printEventList()
            {
                print(TString::Format("Print event list for the cut = %s", name.Data()));
                for (auto& eventid : eventlist)
                {
                    int run = std::get<0>(eventid);
                    int lumi = std::get<1>(eventid);
                    unsigned long long evt = std::get<2>(eventid);
                    TString msg = TString::Format("%d:%d:%llu", run, lumi, evt);
                    std::cout << msg << std::endl;
                }
            }
            void addCut(TString n)
            {
                CutTree* obj = new CutTree(n);
                obj->parent = this;
                obj->parents.push_back(this);
                children.push_back(obj);
            }
            void addSyst(TString syst)
            {
                // If already added ignore
                if (systs.find(syst) != systs.end())
                    return;
                // Syst CutTree object knows the parents, and children, however, the children does not know the syst-counter-part parent, nor the parent knows the syste-counter-part children.
                CutTree* obj = new CutTree(this->name + syst);
                systs[syst] = obj;
                obj->children = this->children;
                obj->parents = this->parents;
                obj->parent = this->parent;
            }
            void addHist1D(TH1F* h, TString var, TString syst)
            {
                if (syst.IsNull())
                    hists1d["Nominal"].push_back(std::make_tuple(h, var));
                else
                    hists1d[syst].push_back(std::make_tuple(h, var));
            }
            void addHist2D(TH2F* h, TString varx, TString vary, TString syst)
            {
                if (syst.IsNull())
                    hists2d["Nominal"].push_back(std::make_tuple(h, varx, vary));
                else
                    hists2d[syst].push_back(std::make_tuple(h, varx, vary));
            }
            CutTree* getCutPointer(TString n)
            {
                // If the name match then return itself
                if (name.EqualTo(n))
                {
                    return this;
                }
                else
                {
                    // Otherwise, loop over the children an if a children has the correct one return the found CutTree
                    for (auto& child : children)
                    {
                        CutTree* c = child->getCutPointer(n);
                        if (c)
                            return c;
                    }
                    return 0;
                }
            }
            // Wrapper to return the object instead of pointer
            CutTree& getCut(TString n)
            {
                CutTree* c = getCutPointer(n);
                if (c)
                {
                    return *c;
                }
                else
                {
                    RooUtil::error(TString::Format("Asked for %s cut, but did not find the cut", n.Data()));
                    return *this;
                }
            }
            std::vector<TString> getCutList(TString n, std::vector<TString> cut_list=std::vector<TString>())
            {
                // Main idea: start from the end node provided by the first argument "n", and work your way up to the root node.
                //
                // The algorithm will first determine whether I am starting from a specific cut requested by the user or within in recursion.
                // If the cut_list.size() == 0, the function is called by the user (since no list is aggregated so far)
                // In that case, first find the pointer to the object we want and set it to "c"
                // If cut_list.size() is non-zero then take this as the cut that I am starting and I go up the chain to aggregate all the cuts prior to the requested cut
                CutTree* c = 0;
                if (cut_list.size() == 0)
                {
                    c = &getCut(n);
                    cut_list.push_back(c->name);
                }
                else
                {
                    c = this;
                    cut_list.push_back(n);
                }
                if (c->parent)
                {
                    return (c->parent)->getCutList((c->parent)->name, cut_list);
                }
                else
                {
                    std::reverse(cut_list.begin(), cut_list.end());
                    return cut_list;
                }
            }
            std::vector<TString> getEndCuts(std::vector<TString> endcuts=std::vector<TString>())
            {
                if (children.size() == 0)
                {
                    endcuts.push_back(name);
                    return endcuts;
                }
                for (auto& child : children)
                    endcuts = child->getEndCuts(endcuts);
                return endcuts;
            }
            std::vector<TString> getCutListBelow(TString n, std::vector<TString> cut_list=std::vector<TString>())
            {
                // Main idea: start from the node provided by the first argument "n", and work your way down to the ends.
                CutTree* c = 0;
                if (cut_list.size() == 0)
                {
                    c = &getCut(n);
                    cut_list.push_back(c->name);
                }
                else
                {
                    c = this;
                    cut_list.push_back(n);
                }
                if (children.size() > 0)
                {
                    for (auto& child : c->children)
                    {
                        cut_list = child->getCutListBelow(child->name, cut_list);
                    }
                    return cut_list;
                }
                else
                {
                    return cut_list;
                }
            }
            void clear()
            {
                pass = false;
                weight = 0;
                for (auto& child : children)
                    child->clear();
            }
            void addSyst(TString syst, std::vector<TString> patterns)
            {
                for (auto& pattern : patterns)
                    if (name.Contains(pattern))
                        addSyst(syst);
                for (auto& child : children)
                    child->addSyst(syst, patterns);
            }
            void evaluate(RooUtil::TTreeX& tx, TString cutsystname="", bool doeventlist=false, bool aggregated_pass=true, float aggregated_weight=1)
            {
#ifdef USE_TTREEX
                evaluate_use_ttreex(tx, cutsystname, doeventlist, aggregated_pass, aggregated_weight);
#else
    #ifdef USE_CUTLAMBDA
                evaluate_use_lambda(tx, cutsystname, doeventlist, aggregated_pass, aggregated_weight);
    #else
                evaluate_use_internal_variable(tx, cutsystname, doeventlist, aggregated_pass, aggregated_weight);
    #endif
#endif
            }
            void evaluate_use_lambda(RooUtil::TTreeX& tx, TString cutsystname="", bool doeventlist=false, bool aggregated_pass=true, float aggregated_weight=1)
            {
                if (!parent)
                {
                    pass = 1;
                    weight = 1;
                }
                else
                {
                    if (cutsystname.IsNull())
                    {
                        if (pass_this_cut_func)
                        {
                            pass = pass_this_cut_func() && aggregated_pass;
                            weight = weight_this_cut_func() * aggregated_weight;
                        }
                        else
                        {
                            TString msg = "cowardly passing the event because cut and weight func not set! cut name = " + name;
                            warning(msg);
                            pass = aggregated_pass;
                            weight = aggregated_weight;
                        }
                    }
                    else
                    {
                        if (systs.find(cutsystname) == systs.end())
                        {
                            if (pass_this_cut_func)
                            {
                                pass = pass_this_cut_func() && aggregated_pass;
                                weight = weight_this_cut_func() * aggregated_weight;
                            }
                            else
                            {
                                TString msg = "cowardly passing the event because cut and weight func not set! cut name = " + name;
                                warning(msg);
                                pass = aggregated_pass;
                                weight = aggregated_weight;
                            }
                        }
                        else
                        {
                            if (systs[cutsystname]->pass_this_cut_func)
                            {
                                pass = systs[cutsystname]->pass_this_cut_func() && aggregated_pass;
                                weight = systs[cutsystname]->weight_this_cut_func() * aggregated_weight;
                            }
                            else
                            {
                                TString msg = "cowardly passing the event because cut and weight func not set! cut name = " + name + " syst name = " + cutsystname;
                                warning(msg);
                                pass = aggregated_pass;
                                weight = aggregated_weight;
                            }
                        }
                    }
                }
                if (doeventlist and pass)
                {
                    if (tx.hasBranch<int>("run") && tx.hasBranch<int>("lumi") && tx.hasBranch<unsigned long long>("evt"))
                    {
                        eventlist.push_back(std::make_tuple(tx.getBranch<int>("run"), tx.getBranch<int>("lumi"), tx.getBranch<unsigned long long>("evt")));
                    }
                }
                for (auto& child : children)
                    child->evaluate_use_lambda(tx, cutsystname, doeventlist, pass, weight);
            }
            void evaluate_use_internal_variable(RooUtil::TTreeX& tx, TString cutsystname="", bool doeventlist=false, bool aggregated_pass=true, float aggregated_weight=1)
            {
                if (!parent)
                {
                    pass = 1;
                    weight = 1;
                }
                else
                {
                    if (cutsystname.IsNull())
                    {
                        pass = pass_this_cut && aggregated_pass;
                        weight = weight_this_cut * aggregated_weight;
                    }
                    else
                    {
                        if (systs.find(cutsystname) == systs.end())
                        {
                            pass = pass_this_cut && aggregated_pass;
                            weight = weight_this_cut * aggregated_weight;
                        }
                        else
                        {
                            pass = systs[cutsystname]->pass_this_cut && aggregated_pass;
                            weight = systs[cutsystname]->weight_this_cut * aggregated_weight;
                        }
                    }
                }
                if (doeventlist and pass)
                {
                    if (tx.hasBranch<int>("run") && tx.hasBranch<int>("lumi") && tx.hasBranch<unsigned long long>("evt"))
                    {
                        eventlist.push_back(std::make_tuple(tx.getBranch<int>("run"), tx.getBranch<int>("lumi"), tx.getBranch<unsigned long long>("evt")));
                    }
                }
                for (auto& child : children)
                    child->evaluate_use_internal_variable(tx, cutsystname, doeventlist, pass, weight);
            }
            void evaluate_use_ttreex(RooUtil::TTreeX& tx, TString cutsystname="", bool doeventlist=false, bool aggregated_pass=true, float aggregated_weight=1)
            {
                if (!parent)
                {
                    pass = tx.getBranch<bool>(name);
                    weight = tx.getBranch<float>(name+"_weight");
                }
                else
                {
                    if (cutsystname.IsNull())
                    {
                        if (!tx.hasBranch<bool>(name))
                            return;
                        pass = tx.getBranch<bool>(name) && aggregated_pass;
                        weight = tx.getBranch<float>(name+"_weight") * aggregated_weight;
                    }
                    else
                    {
                        if (systs.find(cutsystname) == systs.end())
                        {
                            if (!tx.hasBranch<bool>(name))
                                return;
                            pass = tx.getBranch<bool>(name) && aggregated_pass;
                            weight = tx.getBranch<float>(name+"_weight") * aggregated_weight;
                        }
                        else
                        {
                            if (!tx.hasBranch<bool>(name+cutsystname))
                                return;
                            pass = tx.getBranch<bool>(name+cutsystname) && aggregated_pass;
                            weight = tx.getBranch<float>(name+cutsystname+"_weight") * aggregated_weight;
                        }
                    }
                }
                if (doeventlist and pass)
                {
                    if (tx.hasBranch<int>("run") && tx.hasBranch<int>("lumi") && tx.hasBranch<unsigned long long>("evt"))
                    {
                        eventlist.push_back(std::make_tuple(tx.getBranch<int>("run"), tx.getBranch<int>("lumi"), tx.getBranch<unsigned long long>("evt")));
                    }
                }
                for (auto& child : children)
                    child->evaluate_use_ttreex(tx, cutsystname, doeventlist, pass, weight);
            }
            void sortEventList()
            {
                std::sort(eventlist.begin(), eventlist.end(),
                        [](const std::tuple<int, int, unsigned long long>& a, const std::tuple<int, int, unsigned long long>& b)
                        {
                            if (std::get<0>(a) != std::get<0>(b)) return std::get<0>(a) < std::get<0>(b);
                            else if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
                            else if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) < std::get<2>(b);
                            else return true;
                        }
                        );
            }
            void clearEventList()
            {
                eventlist.clear();
            }
            void addEventList(int run, int lumi, unsigned long long evt)
            {
                eventlist.push_back(std::make_tuple(run, lumi, evt));
            }
            void fillHistograms(RooUtil::TTreeX& tx, TString syst, float extrawgt)
            {
                // If the cut didn't pass then stop
                if (!pass)
                    return;

                if (hists1d.size() != 0 or hists2d.size() != 0)
                {
                    TString systkey = syst.IsNull() ? "Nominal" : syst;
                    for (auto& tuple : hists1d[systkey])
                    {
                        TH1F* h = std::get<0>(tuple);
                        TString varname = std::get<1>(tuple);
                        h->Fill(tx.getBranch<float>(varname), weight * extrawgt);
                    }
                    for (auto& tuple : hists2d[systkey])
                    {
                        TH2F* h = std::get<0>(tuple);
                        TString varname = std::get<1>(tuple);
                        TString varnamey = std::get<2>(tuple);
                        h->Fill(tx.getBranch<float>(varname), tx.getBranch<float>(varnamey), weight * extrawgt);
                    }
                }
                for (auto& child : children)
                    child->fillHistograms(tx, syst, extrawgt);

//                if (!parent)
//                {
//                    pass = pass_this_cut;
//                    weight = weight_this_cut;
//                }
//                else
//                {
//                    if (cutsystname.IsNull())
//                    {
//                        pass = pass_this_cut && aggregated_pass;
//                        weight = weight_this_cut * aggregated_weight;
//                    }
//                    else
//                    {
//                        if (systs.find(cutsystname) == systs.end())
//                        {
//                            pass = pass_this_cut && aggregated_pass;
//                            weight = weight_this_cut * aggregated_weight;
//                        }
//                        else
//                        {
//                            pass = systs[cutsystname]->pass_this_cut && aggregated_pass;
//                            weight = systs[cutsystname]->weight_this_cut * aggregated_weight;
//                        }
//                    }
//                }
            }
    };
}

#endif
