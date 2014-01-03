//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_NETWORKOPS_H
#define RIPPLE_NETWORKOPS_H

// Operations that clients may wish to perform against the network
// Master operational handler, server sequencer, network tracker

class Peer;
class LedgerConsensus;

// This is the primary interface into the "client" portion of the program.
// Code that wants to do normal operations on the network such as
// creating and monitoring accounts, creating transactions, and so on
// should use this interface. The RPC code will primarily be a light wrapper
// over this code.
//
// Eventually, it will check the node's operating mode (synched, unsynched,
// etectera) and defer to the correct means of processing. The current
// code assumes this node is synched (and will continue to do so until
// there's a functional network.
//
/** Provides server functionality for clients.

    Clients include backend applications, local commands, and connected
    clients. This class acts as a proxy, fulfilling the command with local
    data if possible, or asking the network and returning the results if
    needed.

    A backend application or local client can trust a local instance of
    rippled / NetworkOPs. However, client software connecting to non-local
    instances of rippled will need to be hardened to protect against hostile
    or unreliable servers.
*/
class NetworkOPs
    : public InfoSub::Source
{
protected:
    explicit NetworkOPs (Stoppable& parent);

public:
    enum Fault
    {
        // exceptions these functions can throw
        IO_ERROR    = 1,
        NO_NETWORK  = 2,
    };

    enum OperatingMode
    {
        // how we process transactions or account balance requests
        omDISCONNECTED  = 0,    // not ready to process requests
        omCONNECTED     = 1,    // convinced we are talking to the network
        omSYNCING       = 2,    // fallen slightly behind
        omTRACKING      = 3,    // convinced we agree with the network
        omFULL          = 4     // we have the ledger and can even validate
    };

    // VFALCO TODO Fix OrderBookDB to not need this unrelated type.
    //
    typedef boost::unordered_map <uint64, InfoSub::wptr> SubMapType;

public:
    // VFALCO TODO Make LedgerMaster a SharedPtr or a reference.
    //
    static NetworkOPs* New (LedgerMaster& ledgerMaster,
        Stoppable& parent, Journal journal);

    virtual ~NetworkOPs () { }

    //--------------------------------------------------------------------------
    //
    // Network information
    //

    // Our best estimate of wall time in seconds from 1/1/2000
    virtual uint32 getNetworkTimeNC () = 0;
    // Our best estimate of current ledger close time
    virtual uint32 getCloseTimeNC () = 0;
    // Use *only* to timestamp our own validation
    virtual uint32 getValidationTimeNC () = 0;
    virtual void closeTimeOffset (int) = 0;
    virtual boost::posix_time::ptime getNetworkTimePT () = 0;
    virtual uint32 getLedgerID (uint256 const& hash) = 0;
    virtual uint32 getCurrentLedgerID () = 0;

    virtual OperatingMode getOperatingMode () = 0;
    virtual std::string strOperatingMode () = 0;
    virtual Ledger::ref     getClosedLedger () = 0;
    virtual Ledger::ref     getValidatedLedger () = 0;
    virtual Ledger::ref     getPublishedLedger () = 0;
    virtual Ledger::ref     getCurrentLedger () = 0;
    virtual Ledger::ref     getCurrentSnapshot () = 0;
    virtual Ledger::pointer getLedgerByHash (uint256 const& hash) = 0;
    virtual Ledger::pointer getLedgerBySeq (const uint32 seq) = 0;
    virtual void            missingNodeInLedger (const uint32 seq) = 0;

    virtual uint256         getClosedLedgerHash () = 0;

    // Do we have this inclusive range of ledgers in our database
    virtual bool haveLedgerRange (uint32 from, uint32 to) = 0;
    virtual bool haveLedger (uint32 seq) = 0;
    virtual uint32 getValidatedSeq () = 0;
    virtual bool isValidated (uint32 seq) = 0;
    virtual bool isValidated (uint32 seq, uint256 const& hash) = 0;
    virtual bool isValidated (Ledger::ref l) = 0;
    virtual bool getValidatedRange (uint32& minVal, uint32& maxVal) = 0;
    virtual bool getFullValidatedRange (uint32& minVal, uint32& maxVal) = 0;

    virtual SerializedValidation::ref getLastValidation () = 0;
    virtual void setLastValidation (SerializedValidation::ref v) = 0;
    virtual SLE::pointer getSLE (Ledger::pointer lpLedger, uint256 const& uHash) = 0;
    virtual SLE::pointer getSLEi (Ledger::pointer lpLedger, uint256 const& uHash) = 0;

    //--------------------------------------------------------------------------
    //
    // Transaction processing
    //

    // must complete immediately
    // VFALCO TODO Make this a TxCallback structure
    typedef std::function<void (Transaction::pointer, TER)> stCallback;
    virtual void submitTransaction (Job&, SerializedTransaction::pointer,
        stCallback callback = stCallback ()) = 0;
    virtual Transaction::pointer submitTransactionSync (Transaction::ref tpTrans,
        bool bAdmin, bool bFailHard, bool bSubmit) = 0;
    virtual void runTransactionQueue () = 0;
    virtual Transaction::pointer processTransaction (Transaction::pointer,
        bool bAdmin, bool bFailHard, stCallback) = 0;
    virtual Transaction::pointer processTransaction (Transaction::pointer transaction,
        bool bAdmin, bool bFailHard) = 0;
    virtual Transaction::pointer findTransactionByID (uint256 const& transactionID) = 0;
    virtual int findTransactionsByDestination (std::list<Transaction::pointer>&,
        const RippleAddress& destinationAccount, uint32 startLedgerSeq,
            uint32 endLedgerSeq, int maxTransactions) = 0;

    //--------------------------------------------------------------------------
    //
    // Account functions
    //

    virtual AccountState::pointer getAccountState (Ledger::ref lrLedger,
        const RippleAddress& accountID) = 0;
    virtual SLE::pointer getGenerator (Ledger::ref lrLedger,
        uint160 const& uGeneratorID) = 0;

    //--------------------------------------------------------------------------
    //
    // Directory functions
    //

    virtual STVector256 getDirNodeInfo (Ledger::ref lrLedger,
        uint256 const& uRootIndex, uint64& uNodePrevious, uint64& uNodeNext) = 0;

    //--------------------------------------------------------------------------
    //
    // Owner functions
    //

    virtual Json::Value getOwnerInfo (Ledger::pointer lpLedger,
        const RippleAddress& naAccount) = 0;

    //--------------------------------------------------------------------------
    //
    // Book functions
    //

    virtual void getBookPage (
        Ledger::pointer lpLedger,
        const uint160& uTakerPaysCurrencyID,
        const uint160& uTakerPaysIssuerID,
        const uint160& uTakerGetsCurrencyID,
        const uint160& uTakerGetsIssuerID,
        const uint160& uTakerID,
        const bool bProof,
        const unsigned int iLimit,
        const Json::Value& jvMarker,
        Json::Value& jvResult) = 0;

    //--------------------------------------------------------------------------

    // ledger proposal/close functions
    virtual void processTrustedProposal (LedgerProposal::pointer proposal,
        boost::shared_ptr<protocol::TMProposeSet> set, RippleAddress nodePublic,
            uint256 checkLedger, bool sigGood) = 0;

    virtual SHAMapAddNode gotTXData (const boost::shared_ptr<Peer>& peer,
        uint256 const& hash, const std::list<SHAMapNode>& nodeIDs,
        const std::list< Blob >& nodeData) = 0;

    virtual bool recvValidation (SerializedValidation::ref val,
        const std::string& source) = 0;

    virtual void takePosition (int seq, SHAMap::ref position) = 0;
    
    virtual SHAMap::pointer getTXMap (uint256 const& hash) = 0;

    virtual bool hasTXSet (const boost::shared_ptr<Peer>& peer,
        uint256 const& set, protocol::TxSetStatus status) = 0;

    virtual void mapComplete (uint256 const& hash, SHAMap::ref map) = 0;
    
    virtual bool stillNeedTXSet (uint256 const& hash) = 0;
    
    // Fetch packs
    virtual void makeFetchPack (Job&, boost::weak_ptr<Peer> peer,
        boost::shared_ptr<protocol::TMGetObjectByHash> request,
        Ledger::pointer wantLedger, Ledger::pointer haveLedger, uint32 uUptime) = 0;

    virtual bool shouldFetchPack (uint32 seq) = 0;
    virtual void gotFetchPack (bool progress, uint32 seq) = 0;
    virtual void addFetchPack (uint256 const& hash, boost::shared_ptr< Blob >& data) = 0;
    virtual bool getFetchPack (uint256 const& hash, Blob& data) = 0;
    virtual int getFetchSize () = 0;
    virtual void sweepFetchPack () = 0;

    // network state machine
    virtual void endConsensus (bool correctLCL) = 0;
    virtual void setStandAlone () = 0;
    virtual void setStateTimer () = 0;
    
    virtual void newLCL (int proposers, int convergeTime, uint256 const& ledgerHash) = 0;
    // VFALCO TODO rename to setNeedNetworkLedger
    virtual void needNetworkLedger () = 0;
    virtual void clearNeedNetworkLedger () = 0;
    virtual bool isNeedNetworkLedger () = 0;
    virtual bool isFull () = 0;
    virtual void setProposing (bool isProposing, bool isValidating) = 0;
    virtual bool isProposing () = 0;
    virtual bool isValidating () = 0;
    virtual bool isFeatureBlocked () = 0;
    virtual void setFeatureBlocked () = 0;
    virtual void consensusViewChange () = 0;
    virtual int getPreviousProposers () = 0;
    virtual int getPreviousConvergeTime () = 0;
    virtual uint32 getLastCloseTime () = 0;
    virtual void setLastCloseTime (uint32 t) = 0;

    virtual Json::Value getConsensusInfo () = 0;
    virtual Json::Value getServerInfo (bool human, bool admin) = 0;
    virtual void clearLedgerFetch () = 0;
    virtual Json::Value getLedgerFetchInfo () = 0;
    virtual uint32 acceptLedger () = 0;

    typedef boost::unordered_map <uint160, std::list<LedgerProposal::pointer> > Proposals;
    virtual Proposals& peekStoredProposals () = 0;

    virtual void storeProposal (LedgerProposal::ref proposal,
        const RippleAddress& peerPublic) = 0;

    virtual uint256 getConsensusLCL () = 0;
    
    virtual void reportFeeChange () = 0;

    //Helper function to generate SQL query to get transactions
    virtual std::string transactionsSQL (std::string selection,
        const RippleAddress& account, int32 minLedger, int32 maxLedger,
        bool descending, uint32 offset, int limit, bool binary,
            bool count, bool bAdmin) = 0;

    // client information retrieval functions
    typedef std::vector< std::pair<Transaction::pointer, TransactionMetaSet::pointer> > AccountTxs;
    virtual AccountTxs getAccountTxs (const RippleAddress& account,
        int32 minLedger, int32 maxLedger,  bool descending, uint32 offset,
            int limit, bool bAdmin) = 0;

    typedef std::vector< std::pair<Transaction::pointer, TransactionMetaSet::pointer> > TxsAccount;
    virtual TxsAccount getTxsAccount (const RippleAddress& account,
        int32 minLedger, int32 maxLedger, bool forward, Json::Value& token,
        int limit, bool bAdmin) = 0;

    typedef boost::tuple<std::string, std::string, uint32> txnMetaLedgerType;
    typedef std::vector<txnMetaLedgerType> MetaTxsList;
    virtual MetaTxsList getAccountTxsB (const RippleAddress& account,
        int32 minLedger, int32 maxLedger,  bool descending,
            uint32 offset, int limit, bool bAdmin) = 0;

    virtual MetaTxsList getTxsAccountB (const RippleAddress& account,
        int32 minLedger, int32 maxLedger,  bool forward,
        Json::Value& token, int limit, bool bAdmin) = 0;

    virtual std::vector<RippleAddress> getLedgerAffectedAccounts (uint32 ledgerSeq) = 0;

    //--------------------------------------------------------------------------
    //
    // Monitoring: publisher side
    //
    virtual void pubLedger (Ledger::ref lpAccepted) = 0;
    virtual void pubProposedTransaction (Ledger::ref lpCurrent,
        SerializedTransaction::ref stTxn, TER terResult) = 0;
};

#endif