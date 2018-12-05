/*
 * This software is not subject to copyright protection
 */
 
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "nullimplsrpi1N.h"

using namespace std;
using namespace SRPI;

NullImplSRPI1N::NullImplSRPI1N() {}

NullImplSRPI1N::~NullImplSRPI1N() {}

ReturnStatus
NullImplSRPI1N::initializeEnrollmentSession(const string &configDir)
{
    this->configDir = configDir;
    return ReturnStatus(ReturnCode::Success);
}

ReturnStatus
NullImplSRPI1N::createTemplate(const SoundRecord &record,
        TemplateRole role,
        vector<uint8_t> &templ)
{
    string blurb{"Long time ago in a galaxy far far away..."};

    templ.resize(blurb.size());
    memcpy(templ.data(), blurb.c_str(), blurb.size());

    return ReturnStatus(ReturnCode::Success);
}

ReturnStatus NullImplSRPI1N::finalizeEnrollment(const std::vector<std::pair<size_t, std::vector<uint8_t>>> &vtempl)
{
    for(size_t i = 0; i < vtempl.size(); ++i)
        labels.push_back(vtempl[i].first);
    return ReturnCode::Success;
}

ReturnStatus
NullImplSRPI1N::initializeIdentificationSession(const string &configDir)
{
    this->configDir = configDir;
    return ReturnCode::Success;
}

ReturnStatus
NullImplSRPI1N::identifyTemplate(
        const vector<uint8_t> &idTemplate,
        const size_t candidateListLength,
        vector<Candidate> &candidateList,
        bool &decision)
{
    for(size_t i = 0; i < candidateListLength; i++) {
        if(i < labels.size())
            candidateList.push_back(Candidate(true, labels[i], candidateListLength-i));
        else
            candidateList.push_back(Candidate(false, i, candidateListLength-i));
    }

    decision = true;
    return ReturnCode::Success;
}

shared_ptr<IdentInterface>
IdentInterface::getImplementation()
{
    return make_shared<NullImplSRPI1N>();
}

