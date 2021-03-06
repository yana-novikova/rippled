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

#ifndef RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED

#include <ripple/app/ledger/LedgerEntrySet.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <cstdint>
#include <beast/cxx14/memory.h> // <memory>

namespace ripple {

/** Wraps a trust line SLE for convenience.
    The complication of trust lines is that there is a
    "low" account and a "high" account. This wraps the
    SLE and expresses its data from the perspective of
    a chosen account on the line.
*/
// VFALCO TODO Rename to TrustLine
class RippleState
{
public:
    // VFALCO Why is this shared_ptr?
    using pointer = std::shared_ptr <RippleState>;

public:
    RippleState() = delete;

    virtual ~RippleState() = default;

    static RippleState::pointer makeItem(
        Account const& accountID,
        std::shared_ptr<SLE const> sle);

    // Must be public, for make_shared
    RippleState (std::shared_ptr<SLE const>&& sle,
        Account const& viewAccount);

    /** Returns the state map key for the ledger entry. */
    uint256
    key() const
    {
        return mLedgerEntry->getIndex();
    }

    // VFALCO Take off the "get" from each function name

    Account const& getAccountID () const
    {
        return  mViewLowest ? mLowID : mHighID;
    }

    Account const& getAccountIDPeer () const
    {
        return !mViewLowest ? mLowID : mHighID;
    }

    // True, Provided auth to peer.
    bool getAuth () const
    {
        return mFlags & (mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getAuthPeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getNoRipple () const
    {
        return mFlags & (mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    bool getNoRipplePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    /** Have we set the freeze flag on our peer */
    bool getFreeze () const
    {
        return mFlags & (mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    /** Has the peer set the freeze flag on us */
    bool getFreezePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    STAmount const& getBalance () const
    {
        return mBalance;
    }

    STAmount const& getLimit () const
    {
        return  mViewLowest ? mLowLimit : mHighLimit;
    }

    STAmount const& getLimitPeer () const
    {
        return !mViewLowest ? mLowLimit : mHighLimit;
    }

    std::uint32_t getQualityIn () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityIn : mHighQualityIn));
    }

    std::uint32_t getQualityOut () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityOut : mHighQualityOut));
    }

    Json::Value getJson (int);

private:
    std::shared_ptr<SLE const> mLedgerEntry;

    bool                            mViewLowest;

    std::uint32_t                   mFlags;

    STAmount const&                 mLowLimit;
    STAmount const&                 mHighLimit;

    Account const&                  mLowID;
    Account const&                  mHighID;

    std::uint64_t                   mLowQualityIn;
    std::uint64_t                   mLowQualityOut;
    std::uint64_t                   mHighQualityIn;
    std::uint64_t                   mHighQualityOut;

    STAmount                        mBalance;
};

std::vector <RippleState::pointer>
getRippleStateItems (
    Account const& accountID,
    Ledger::ref ledger);

} // ripple

#endif
