#include "module.h"

void RooUtil::Module::SetTTreeX(RooUtil::TTreeX* tx_) { tx = tx_; }
void RooUtil::Module::AddOutput() {}
void RooUtil::Module::FillOutput() {}

RooUtil::Processor::Processor(RooUtil::TTreeX* tx_)
{
    tx = tx_;
}

void RooUtil::Processor::AddModule(RooUtil::Module* module)
{
    modules.emplace_back(module);
}

void RooUtil::Processor::AddOutputs()
{
    for (auto& module: modules)
    {
        module->SetTTreeX(tx);
        module->AddOutput();
    }
    tx->clear();
}

void RooUtil::Processor::FillOutputs()
{
    for (auto& module: modules)
        module->FillOutput();
    tx->fill();
    tx->clear();
}
