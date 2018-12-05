/*
 * This software is not subject to copyright protection and is in the public domain.
 */

#ifndef NULLIMPLSRPI1N_H_
#define NULLIMPLSRPI1N_H_

#include "srpi.h"

/*
 * Declare the implementation class of the SRPI IDENT (1:N) Interface
 */
namespace SRPI {
    class NullImplSRPI1N : public SRPI::IdentInterface {
public:

    NullImplSRPI1N();
    ~NullImplSRPI1N() override;

    ReturnStatus
    initializeEnrollmentSession(const std::string &configDir) override;

    ReturnStatus
    createTemplate(
            const SoundRecord &record,
            TemplateRole role,
            std::vector<uint8_t> &templ) override;

    ReturnStatus
    finalizeEnrollment(
            const std::vector<std::pair<size_t,std::vector<uint8_t>>> &vtempl) override;

    ReturnStatus
    initializeIdentificationSession(
            const std::string &configDir) override;

    ReturnStatus
    identifyTemplate(const std::vector<uint8_t> &idTemplate,
            const size_t candidateListLength,
            std::vector<Candidate> &candidateList,
            bool &decision) override;

    static std::shared_ptr<SRPI::IdentInterface>
    getImplementation();

private:
    std::string configDir;
    std::string enrollDir;
    std::vector<size_t> labels;
    int counter;
    // Some other members
};
}

#endif /* NULLIMPLSRPI1N_H_ */
