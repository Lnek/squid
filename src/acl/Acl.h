/*
 * Copyright (C) 1996-2016 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_H
#define SQUID_ACL_H

#include "acl/forward.h"
#include "base/CharacterSet.h"
#include "cbdata.h"
#include "defines.h"
#include "dlink.h"
#include "SBufList.h"

#include <ostream>
#include <string>
#include <vector>

class ConfigParser;

typedef char ACLFlag;
// ACLData Flags
#define ACL_F_REGEX_CASE 'i'
#define ACL_F_NO_LOOKUP 'n'
#define ACL_F_STRICT 's'
#define ACL_F_SUBSTRING 'm'
#define ACL_F_END '\0'

/**
 * \ingroup ACLAPI
 * Used to hold a list of one-letter flags which can be passed as parameters
 * to acls  (eg '-i', '-n' etc)
 */
class ACLFlags
{
public:
    enum Status
    {
        notSupported,
        noParameter,
        parameterOptional,
        parameterRequired
    };

    explicit ACLFlags(const ACLFlag flags[]) : supported_(flags), flags_(0), delimiters_(nullptr) {}
    ACLFlags() : flags_(0), delimiters_(nullptr) {}
    ~ACLFlags();
    /// \return a Status for the given ACLFlag.
    Status flagStatus(const ACLFlag f) const;
    /// \return true if the parameter for the given flag is acceptable.
    bool parameterSupported(const ACLFlag f, const SBuf &val) const;
    /// Set the given flag
    void makeSet(const ACLFlag f, const SBuf &param = SBuf(""));
    void makeUnSet(const ACLFlag f); ///< Unset the given flag
    /// \return true if the given flag is set.
    bool isSet(const ACLFlag f) const { return flags_ & flagToInt(f);}
    /// \return the parameter value of the given flag if set.
    SBuf parameter(const ACLFlag f) const;
    /// \return ACL_F_SUBSTRING parameter value(if set) converted to CharacterSet.
    const CharacterSet *delimiters();
    /// Parse optional flags given in the form -[A..Z|a..z]
    void parseFlags();
    const char *flagsStr() const; ///< Convert the flags to a string representation
    /**
     * Lexical analyzer for ACL flags
     *
     * Support tokens in the form:
     *   flag := '-' [A-Z|a-z]+ ['=' parameter ]
     * Each token consist by one or more single-letter flags, which may
     * followed by a parameter string.
     * The parameter can belongs only to the last flag in token.
     */
    class FlagsTokenizer
    {
    public:
        FlagsTokenizer();
        ACLFlag nextFlag(); ///< The next flag or '\0' if finished
        /// \return true if a parameter follows the last parsed flag.
        bool hasParameter() const;
        /// \return the parameter of last parsed flag, if exist.
        SBuf getParameter() const;

    private:
        /// \return true if the current token parsing is finished.
        bool needNextToken() const;
        /// Peeks at the next token and return false if the next token
        /// is not flag, or a '--' is read.
        bool nextToken();

        char *tokPos;
    };

private:
    /// Convert a flag to a 64bit unsigned integer.
    /// The characters from 'A' to 'z' represented by the values from 65 to 122.
    /// They are 57 different characters which can be fit to the bits of an 64bit
    /// integer.
    uint64_t flagToInt(const ACLFlag f) const {
        assert('A' <= f && f <= 'z');
        return ((uint64_t)1 << (f - 'A'));
    }

    std::string supported_; ///< The supported character flags
    uint64_t flags_; ///< The flags which are set
    static const uint32_t FlagIndexMax = 'z' - 'A';
    std::map<ACLFlag, SBuf> flagParameters_;
    CharacterSet *delimiters_;
public:
    static const ACLFlag NoFlags[1]; ///< An empty flags list
};

/// A configurable condition. A node in the ACL expression tree.
/// Can evaluate itself in FilledChecklist context.
/// Does not change during evaluation.
/// \ingroup ACLAPI
class ACL
{

public:
    void *operator new(size_t);
    void operator delete(void *);

    static ACL *Factory(char const *);
    static void ParseAclLine(ConfigParser &parser, ACL ** head);
    static void Initialize();
    static ACL *FindByName(const char *name);

    ACL();
    explicit ACL(const ACLFlag flgs[]);
    virtual ~ACL();

    /// sets user-specified ACL name and squid.conf context
    void context(const char *name, const char *configuration);

    /// Orchestrates matching checklist against the ACL using match(),
    /// after checking preconditions and while providing debugging.
    /// \return true if and only if there was a successful match.
    /// Updates the checklist state on match, async, and failure.
    bool matches(ACLChecklist *checklist) const;

    virtual ACL *clone() const = 0;

    /// parses node represenation in squid.conf; dies on failures
    virtual void parse() = 0;
    virtual char const *typeString() const = 0;
    virtual bool isProxyAuth() const;
    virtual SBufList dump() const = 0;
    virtual bool empty() const = 0;
    virtual bool valid() const;

    int cacheMatchAcl(dlink_list * cache, ACLChecklist *);
    virtual int matchForCache(ACLChecklist *checklist);

    virtual void prepareForUse() {}

    char name[ACL_NAME_SZ];
    char *cfgline;
    ACL *next; // XXX: remove or at least use refcounting
    ACLFlags flags; ///< The list of given ACL flags
    bool registered; ///< added to the global list of ACLs via aclRegister()

public:

    class Prototype
    {

    public:
        Prototype();
        Prototype(ACL const *, char const *);
        ~Prototype();
        static bool Registered(char const *);
        static ACL *Factory(char const *);

    private:
        ACL const *prototype;
        char const *typeString;

    private:
        static std::vector<Prototype const *> * Registry;
        static void *Initialized;
        typedef std::vector<Prototype const*>::iterator iterator;
        typedef std::vector<Prototype const*>::const_iterator const_iterator;
        void registerMe();
    };

private:
    /// Matches the actual data in checklist against this ACL.
    virtual int match(ACLChecklist *checklist) = 0; // XXX: missing const

    /// whether our (i.e. shallow) match() requires checklist to have a AccessLogEntry
    virtual bool requiresAle() const;
    /// whether our (i.e. shallow) match() requires checklist to have a request
    virtual bool requiresRequest() const;
    /// whether our (i.e. shallow) match() requires checklist to have a reply
    virtual bool requiresReply() const;
};

/// \ingroup ACLAPI
typedef enum {
    // Authorization ACL result states
    ACCESS_DENIED,
    ACCESS_ALLOWED,
    ACCESS_DUNNO,

    // Authentication ACL result states
    ACCESS_AUTH_REQUIRED,    // Missing Credentials
} aclMatchCode;

/// \ingroup ACLAPI
/// ACL check answer; TODO: Rename to Acl::Answer
class allow_t
{
public:
    // not explicit: allow "aclMatchCode to allow_t" conversions (for now)
    allow_t(const aclMatchCode aCode, int aKind = 0): code(aCode), kind(aKind) {}

    allow_t(): code(ACCESS_DUNNO), kind(0) {}

    bool operator ==(const aclMatchCode aCode) const {
        return code == aCode;
    }

    bool operator !=(const aclMatchCode aCode) const {
        return !(*this == aCode);
    }

    bool operator ==(const allow_t allow) const {
        return code == allow.code && kind == allow.kind;
    }

    operator aclMatchCode() const {
        return code;
    }

    aclMatchCode code; ///< ACCESS_* code
    int kind; ///< which custom access list verb matched
};

inline std::ostream &
operator <<(std::ostream &o, const allow_t a)
{
    switch (a) {
    case ACCESS_DENIED:
        o << "DENIED";
        break;
    case ACCESS_ALLOWED:
        o << "ALLOWED";
        break;
    case ACCESS_DUNNO:
        o << "DUNNO";
        break;
    case ACCESS_AUTH_REQUIRED:
        o << "AUTH_REQUIRED";
        break;
    }
    return o;
}

/// \ingroup ACLAPI
class acl_proxy_auth_match_cache
{
    MEMPROXY_CLASS(acl_proxy_auth_match_cache);

public:
    acl_proxy_auth_match_cache(int matchRv, void * aclData) :
        matchrv(matchRv),
        acl_data(aclData)
    {}

    dlink_node link;
    int matchrv;
    void *acl_data;
};

/// \ingroup ACLAPI
/// XXX: find a way to remove or at least use a refcounted ACL pointer
extern const char *AclMatchedName;  /* NULL */

#endif /* SQUID_ACL_H */

