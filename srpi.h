/*
 * Speech Recognition Performance Identification
 *
 * This file contains SRPI API description along with the
 * supplied data types definitions and API's interface declaration
 * to be implemented by the speech recognition software vendors
 *
 * This software is not subject to copyright protection
 */

#ifndef SRPI_H_
#define SRPI_H_

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef BUILD_SHARED_LIBRARY
    #ifdef Q_OS_LINUX
        #define DLLSPEC __attribute__((visibility("default")))
    #else
        #define DLLSPEC __declspec(dllexport)
    #endif
#else
    #ifdef Q_OS_LINUX
        #define DLLSPEC
    #else
        #define DLLSPEC __declspec(dllimport)
    #endif
#endif

namespace SRPI {
	
/** =================================================================
 * @brief
 * Struct representing a single SoundRecord
 */
typedef struct SoundRecord {
    /** Number of frames in record (one frame contains single sample count for all channels)*/
    uint32_t length;
    /** Number of channels in record*/
    uint8_t channels;
    /** Number of bits per sample. Legal values are 8, 16, 24 and 32*/
    uint8_t depth;
    /** Managed pointer to record data*/
    std::shared_ptr<uint8_t> data;

    /** Audio format notes
     * Sample of the SoundRecord always represents signed integer value, that stored in memory as sucessive bytes set with little endian layout
     * Little endian layout means that least significant byte stored in the smallest address
     *
     * Example: if depth == 16 and channels == 2 then SoundRecord's data points to [lA][mA][lB][mB][lA][mA][lB][mB]...,
     * where l - least significant, m - most significant, A - first channel, B - second channel
     * */

    SoundRecord() :
        length{0},
        channels{0},
        depth{0}
        {}

    SoundRecord(
        uint32_t length,
        uint8_t channels,
        uint8_t depth,
        const std::shared_ptr<uint8_t> &data
        ) :
        length{length},
        channels{channels},
        depth{depth},
        data{data}
        {}

    /** @brief This function returns the size of the SoundRecord data*/
    size_t
    size() const { return (length * channels * (depth / 8)); }
} SoundRecord;


/** =================================================================
 * Labels describing the type/role of the template 
 * to be generated (provided as input to template generation)
 */
enum class TemplateRole {
    /** Enrollment template for 1:N */
    Enrollment_1N,
    /** Search template for 1:N */
    Search_1N
};

/** =================================================================
 * @brief
 * Return codes for functions specified in this API
 */
enum class ReturnCode {
    /** Success */
    Success = 0,
    /** Error reading configuration files */
    ConfigError,
    /** Error writing enrollment data */
    EnrollDirError,
    /** Elective refusal to produce a template */
    TemplateCreationError,
    /** There was a problem setting or accessing the GPU */
    GPUError,
    /** Vendor-defined failure */
    VendorError
};

/** Output stream operator for a ReturnCode object. */
inline std::ostream&
operator<<(
    std::ostream &s,
    const ReturnCode &rc)
{
    switch (rc) {
    case ReturnCode::Success:
        return (s << "Success");
    case ReturnCode::ConfigError:
        return (s << "Error reading configuration files");
    case ReturnCode::TemplateCreationError:
        return (s << "Elective refusal to produce a template");   
    case ReturnCode::GPUError:
        return (s << "Problem setting or accessing the GPU");
    case ReturnCode::VendorError:
        return (s << "Vendor-defined error");
    default:
        return (s << "Undefined error");
    }
}

/** =================================================================
 * @brief
 * A structure to contain information about a failure by the software
 * under test
 *
 * @details
 * An object of this class allows the software to return some information
 * from a function call. The string within this object can be optionally
 * set to provide more information for debugging etc. The status code
 * will be set by the function to Success on success, or one of the
 * other codes on failure
 */
typedef struct ReturnStatus {
    /** @brief Return status code */
    ReturnCode code;
    /** @brief Optional information string */
    std::string info;

    ReturnStatus() {}
    /**
     * @brief
     * Create a ReturnStatus object.
     *
     * @param[in] code
     * The return status code; required.
     * @param[in] info
     * The optional information string.
     */
    ReturnStatus(
        const ReturnCode code,
        const std::string &info = ""
        ) :
        code{code},
        info{info}
        {}
} ReturnStatus;

/**
 * @brief
 * Data structure for result of an identification search
 */
typedef struct Candidate {
    /** @brief If the candidate is valid, this should be set to true. If
     * the candidate computation failed, this should be set to false. */
    bool isAssigned;

    /** @brief The template label from the enrollment set*/
    size_t label;

    /** @brief Measure of similarity between the identification template
     * and the enrolled candidate.  Higher scores mean more likelihood that
     * the samples are of the same person.  An algorithm is free to assign
     * any value to a candidate.
     */
    double similarityScore;

    Candidate() :
        isAssigned{false},
        label(0),
        similarityScore{0.0}
        {}

    Candidate(
        bool isAssigned,
        size_t label,
        double similarityScore) :
        isAssigned{isAssigned},
        label(label),
        similarityScore{similarityScore}
        {}
} Candidate;

/** =================================================================
 * @brief
 * The interface to SRPI 1:N implementation (1:N means one to many recognition scheme)
 *
 * @details
 * The submission software under test will implement this interface by
 * sub-classing this class and implementing each method therein.
 */
class DLLSPEC IdentInterface {
public:
    virtual ~IdentInterface() {}

    /** @brief This function initializes the implementation under test and sets
     * all needed parameters.
     *
     * @details This function will be called N=1 times by the SRPITest application
     *
     * @param[in] configDir
     * A read-only directory containing any developer-supplied configuration
     * parameters or run-time data files.
     */
    virtual ReturnStatus
    initializeEnrollmentSession(
        const std::string &configDir) = 0;

    /**
     * @brief This function takes an Sound record and outputs a template
     *
     * @details For enrollment templates: If the function
     * executes correctly (i.e. returns a successful exit status), the SRPITest
     * calling application will store the template.  The SRPITest application will
     * concatenate the templates and pass the result to the enrollment
     * finalization function.  When the implementation fails to produce a
     * template, it shall still return a blank template (which can be zero
     * bytes in length). The template will be included in the
     * enrollment database/manifest like all other enrollment templates, but
     * is not expected to contain any feature information.
     * <br>For identification templates: If the function returns a
     * non-successful return status, the output template will be not be used
     * in subsequent search operations.
     *
     * @param[in] record
     * The input sound record.
     * @param[in] templateType
     * A value from the TemplateRole enumeration that indicates the intended
     * usage of the template to be generated.  In this case, either an
     * enrollment template used for gallery enrollment or an identification
     * template used for search.
     * @param[out] templ
     * The output template.  The format is entirely unregulated.  This will be
     * an empty vector when passed into the function, and the implementation can
     * resize and populate it with the appropriate data.
     */
    virtual ReturnStatus
    createTemplate(
        const SoundRecord &record,
        TemplateRole role,
        std::vector<uint8_t> &templ) = 0;

    /**
     * @brief This function will be called after all enrollment templates have
     * been created and freezes the enrollment data.
     * After this call, the enrollment dataset will be read-only.
     *
     * @details This function allows the implementation to conduct,
     * for example, statistical processing of the feature data, indexing and
     * data re-organization.  The function may create its own data structure.
     * It may increase or decrease the size of the stored data.  No output is
     * expected from this function, except a return code.  The function will
     * generally be called after all the enrollment processes are complete.
     * NOTE: Implementations shall not move the input data.  Implementations
     * shall not point to the input data.
     * Implementations should not assume the input data would be readable
     * after the call.  Implementations must,
     * <b>at a minimum, copy the input data</b> or otherwise extract what is
     * needed for search.
     *
     * @param[in] vtempl
     * Vector of enrollment templates along with the labels identifiers
     */
    virtual ReturnStatus
    finalizeEnrollment(
        const std::vector<std::pair<size_t,std::vector<uint8_t>>> &vtempl) = 0;

    /** @brief This function will be called once prior to one or more calls to
     * identifyTemplate().  The function might set static internal variables
     * so that the enrollment database is available to the subsequent
     * identification searches.
     *
     * @param[in] configDir
     * A read-only directory containing any developer-supplied configuration
     * parameters or run-time data files.
     */
    virtual ReturnStatus
    initializeIdentificationSession(
        const std::string &configDir) = 0;

    /** @brief This function searches an identification template against the
     * enrollment set, and outputs a
     * vector containing candidateListLength Candidates.
     *
     * @details Each candidate shall be populated by the implementation
     * and added to candidateList.  Note that candidateList will be an empty
     * vector when passed into this function.  The candidates shall appear in
     * descending order of similarity score - i.e. most similar entries appear
     * first.
     *
     * @param[in] idTemplate
     * A template from createTemplate().  If the value returned by that
     * function was non-successful, the contents of idTemplate will not be
     * used, and this function will not be called.
     * @param[in] candidateListLength
     * The number of candidates the search should return.
     * @param[out] candidateList
     * Each candidate shall be populated by the implementation.  The candidates
     * shall appear in descending order of similarity score - i.e. most similar
     * entries appear first.
     * @param[out] decision
     * A best guess at whether there is a mate within the enrollment database.
     * If there was a mate found, this value should be set to true, Otherwise, false.
     * Many such decisions allow a single point to be plotted alongside a ROC curve.
     */
    virtual ReturnStatus
    identifyTemplate(
        const std::vector<uint8_t> &idTemplate,
        const size_t candidateListLength,
        std::vector<Candidate> &candidateList,
        bool &decision) = 0;

    /**
     * @brief
     * Factory method to return a managed pointer to the IdentInterface
     * object.
     *
     * @details
     * This function is implemented by the submitted library and must return
     * a managed pointer to the IdentInterface object.
     *
     * @note
     * A possible implementation might be:
     * return (std::make_shared<Implementation>());
     */
    static std::shared_ptr<IdentInterface>
    getImplementation();
};
/* End of IdentInterface */
}

#endif /* SRPI_H_ */

