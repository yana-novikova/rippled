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

#include <BeastConfig.h>
#include <ripple/basics/Log.h>
#include <ripple/test/jtx.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/TxFlags.h>
#include <beast/hash/uhash.h>
#include <beast/unit_test/suite.h>
#include <boost/lexical_cast.hpp>
#include <utility>

namespace ripple {
namespace test {

class Env_test : public beast::unit_test::suite
{
public:
    template <class T>
    static
    std::string
    to_string (T const& t)
    {
        return boost::lexical_cast<
            std::string>(t);
    }

    // Declarations in Account.h
    void
    testAccount()
    {
        using namespace jtx;
        {
            Account a;
            Account b(a);
            a = b;
            a = std::move(b);
            Account c(std::move(a));
        }
        Account("alice");
        Account("alice", KeyType::secp256k1);
        Account("alice", KeyType::ed25519);
        auto const gw = Account("gw");
        [](ripple::Account){}(gw);
        auto const USD = gw["USD"];
        void(Account("alice") < gw);
        std::set<Account>().emplace(gw);
        std::unordered_set<Account,
            beast::uhash<>>().emplace("alice");
    }

    // Declarations in amount.h
    void
    testAmount()
    {
        using namespace jtx;

        PrettyAmount(0);
        PrettyAmount(1);
        PrettyAmount(0u);
        PrettyAmount(1u);
        PrettyAmount(-1);
        static_assert(! std::is_constructible<
            PrettyAmount, char>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, unsigned char>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, short>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, unsigned short>::value, "");

        try
        {
            XRP(0.0000001);
            fail("missing exception");
        }
        catch(std::domain_error const&)
        {
            pass();
        }
        XRP(-0.000001);
        try
        {
            XRP(-0.0000009);
            fail("missing exception");
        }
        catch(std::domain_error const&)
        {
            pass();
        }

        expect(to_string(XRP(5)) == "5 XRP");
        expect(to_string(XRP(.80)) == "0.8 XRP");
        expect(to_string(XRP(.005)) == "5000 drops");
        expect(to_string(XRP(0.1)) == "0.1 XRP");
        expect(to_string(drops(10)) == "10 drops");
        expect(to_string(drops(123400000)) == "123.4 XRP");
        expect(to_string(XRP(-5)) == "-5 XRP");
        expect(to_string(XRP(-.99)) == "-0.99 XRP");
        expect(to_string(XRP(-.005)) == "-5000 drops");
        expect(to_string(XRP(-0.1)) == "-0.1 XRP");
        expect(to_string(drops(-10)) == "-10 drops");
        expect(to_string(drops(-123400000)) == "-123.4 XRP");

        expect(XRP(1) == drops(1000000));
        expect(XRP(1) == STAmount(1000000));
        expect(STAmount(1000000) == XRP(1));

        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        expect(to_string(USD(0)) == "0/USD(gw)");
        expect(to_string(USD(10)) == "10/USD(gw)");
        expect(to_string(USD(-10)) == "-10/USD(gw)");
        expect(USD(0) == STAmount(USD, 0));
        expect(USD(1) == STAmount(USD, 1));
        expect(USD(-1) == STAmount(USD, -1));

        auto const get = [](AnyAmount a){ return a; };
        expect(! get(USD(10)).is_any);
        expect(get(any(USD(10))).is_any);
    }

    // Test Env
    void
    testEnv()
    {
        using namespace jtx;
        auto const n = XRP(10000);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const alice = Account("alice");

        // fund
        {
            Env env(*this);

            // variadics
            env.fund(n, "alice");
            env.fund(n, "bob", "carol");
            env.fund(n, "dave", noripple("eric"));
            env.fund(n, "fred", noripple("gary", "hank"));
            env.fund(n, noripple("irene"));
            env.fund(n, noripple("jim"), "karen");
            env.fund(n, noripple("lisa", "mary"));

            // flags
            env.fund(n, noripple("xavier"));
            env.require(nflags("xavier", asfDefaultRipple));
            env.fund(n, "yana");
            env.require(flags("yana", asfDefaultRipple));

            // fund always autofills
            env.auto_fee(false);
            env.auto_seq(false);
            env.auto_sig(false);
            env.fund(n, "zeke");
            env.require(balance("zeke", n));
        }

        // trust
        {
            Env env(*this);
            env.fund(n, "alice", "bob", gw);
            env(trust("alice", USD(100)), require(lines("alice", 1)));

            // trust always autofills
            env.auto_fee(false);
            env.auto_seq(false);
            env.auto_sig(false);
            env.trust(USD(100), "bob");
            env.require(lines("bob", 1));
        }

        // balance
        {
            Env env(*this);
            expect(env.balance(alice) == 0);
            expect(env.balance(alice, USD) != 0);
            expect(env.balance(alice, USD) == USD(0));
            env.fund(n, alice, gw);
            expect(env.balance(alice) == n);
            expect(env.balance(gw) == n);
            env.trust(USD(1000), alice);
            env(pay(gw, alice, USD(10)));
            expect(to_string(env.balance("alice", USD)) == "10/USD(gw)");
            expect(to_string(env.balance(gw, alice["USD"])) == "-10/USD(alice)");
        }

        // seq
        {
            Env env(*this);
            env.fund(n, noripple("alice", gw));
            expect(env.seq("alice") == 1);
            expect(env.seq(gw) == 1);
        }

        // autofill
        {
            Env env(*this);
            env.fund(n, "alice");
            env.auto_fee(false);
            env.require(balance("alice", n));
            env(noop("alice"), fee(1),                  ter(telINSUF_FEE_P));
            env(noop("alice"), seq(none),               ter(temMALFORMED));
            env(noop("alice"), seq(none), fee(10),      ter(temMALFORMED));
            env(noop("alice"), fee(none),               ter(temMALFORMED));
            env(noop("alice"), sig(none),               ter(temMALFORMED));
            env(noop("alice"), fee(autofill));
            env.auto_seq(false);
            env(noop("alice"), fee(autofill), seq(autofill));
            env.auto_sig(false);
            env(noop("alice"), fee(autofill), seq(autofill), sig(autofill));
        }
    }

    // Env::require
    void
    testRequire()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        env.require(balance("alice", none));
        env.require(balance("alice", XRP(none)));
        env.fund(XRP(10000), "alice", gw);
        env.require(balance("alice", USD(none)));
        env.trust(USD(100), "alice");
        env.require(balance("alice", XRP(10000) - drops(10)));
        env.require(balance("alice", USD(0)));
        env(pay(gw, "alice", USD(10)), require(balance("alice", USD(10))));

        env.require(nflags("alice", asfRequireDest));
        env(fset("alice", asfRequireDest), require(flags("alice", asfRequireDest)));
        env(fclear("alice", asfRequireDest), require(nflags("alice", asfRequireDest)));
    }

    // Signing with secp256k1 and ed25519 keys
    void
    testKeyType()
    {
        using namespace jtx;

        Env env(*this);
        Account const alice("alice", KeyType::ed25519);
        Account const bob("bob", KeyType::secp256k1);
        Account const carol("carol");
        env.fund(XRP(10000), alice, bob);

        // Master key only
        env(noop(alice));
        env(noop(bob));
        env(noop(alice), sig("alice"),                          ter(tefBAD_AUTH_MASTER));
        env(noop(alice), sig(Account("alice",
            KeyType::secp256k1)),                               ter(tefBAD_AUTH_MASTER));
        env(noop(bob), sig(Account("bob",
            KeyType::ed25519)),                                 ter(tefBAD_AUTH_MASTER));
        env(noop(alice), sig(carol),                            ter(tefBAD_AUTH_MASTER));

        // Master and Regular key
        env(regkey(alice, bob));
        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice));

        // Regular key only
        env(fset(alice, asfDisableMaster), sig(alice));
        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice),                            ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(alice),        ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(bob));
        env(noop(alice), sig(alice));
    }

    // Payment basics
    void
    testPayments()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env(pay(env.master, "alice", XRP(1000)), fee(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), fee(1),        ter(telINSUF_FEE_P));
        env(pay(env.master, "alice", XRP(1000)), seq(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), seq(2),        ter(terPRE_SEQ));
        env(pay(env.master, "alice", XRP(1000)), sig(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), sig("bob"),    ter(tefBAD_AUTH_MASTER));

        env(pay(env.master, "dilbert", XRP(1000)), sig(env.master));

        env.fund(XRP(10000), "alice", "bob", "carol", gw);
        env.require(balance("alice", XRP(10000)));
        env.require(balance("bob", XRP(10000)));
        env.require(balance("carol", XRP(10000)));
        env.require(balance(gw, XRP(10000)));

        env.trust(USD(100), "alice", "bob", "carol");
        env.require(owners("alice", 1), lines("alice", 1));
        env(rate(gw, 1.05));

        env(pay(gw, "carol", USD(50)));
        env.require(balance("carol", USD(50)));
        env.require(balance(gw, Account("carol")["USD"](-50)));

        env(offer("carol", XRP(50), USD(50)), require(owners("carol", 2)));
        env(pay("alice", "bob", any(USD(10))),                  ter(tecPATH_DRY));
        env(pay("alice", "bob", any(USD(10))),
            paths(XRP), sendmax(XRP(10)),                       ter(tecPATH_PARTIAL));
        env(pay("alice", "bob", any(USD(10))), paths(XRP),
            sendmax(XRP(20)));
        env.require(balance("bob", USD(10)));
        env.require(balance("carol", USD(39.5)));

        env.memoize("eric");
        env(regkey("alice", "eric"));
        env(noop("alice"));
        env(noop("alice"), sig("alice"));
        env(noop("alice"), sig("eric"));
        env(noop("alice"), sig("bob"),                          ter(tefBAD_AUTH));
        env(fset("alice", asfDisableMaster),                    ter(tecNEED_MASTER_KEY));
        env(fset("alice", asfDisableMaster), sig("eric"),       ter(tecNEED_MASTER_KEY));
        env.require(nflags("alice", asfDisableMaster)); 
        env(fset("alice", asfDisableMaster), sig("alice"));
        env.require(flags("alice", asfDisableMaster));
        env(regkey("alice", disabled),                          ter(tecMASTER_DISABLED));
        env(noop("alice"));
        env(noop("alice"), sig("alice"),                        ter(tefMASTER_DISABLED));
        env(noop("alice"), sig("eric"));
        env(noop("alice"), sig("bob"),                          ter(tefBAD_AUTH));
        env(fclear("alice", asfDisableMaster), sig("bob"),      ter(tefBAD_AUTH));
        env(fclear("alice", asfDisableMaster), sig("alice"),    ter(tefMASTER_DISABLED));
        env(fclear("alice", asfDisableMaster));
        env.require(nflags("alice", asfDisableMaster));
        env(regkey("alice", disabled));
        env(noop("alice"), sig("eric"),                         ter(tefBAD_AUTH_MASTER));
        env(noop("alice"));
    }

    // Multi-sign basics
    void
    testMultiSign()
    {
        using namespace jtx;

        Env env(*this);
        env.fund(XRP(10000), "alice");
        env(signers("alice", 1,
            { { "alice", 1 }, { "bob", 2 } }),                  ter(temBAD_SIGNER));
        env(signers("alice", 1,
            { { "bob", 1 }, { "carol", 2 } }));
        env(noop("alice"));

        env(noop("alice"), msig("bob"));
        env(noop("alice"), msig("carol"));
        env(noop("alice"), msig("bob", "carol"));
        env(noop("alice"), msig("bob", "carol", "dilbert"),     ter(tefBAD_SIGNATURE));
    }

    // Two level Multi-sign
    void
    testMultiSign2()
    {
        using namespace jtx;

        Env env(*this);
        env.fund(XRP(10000), "alice", "bob", "carol");
        env.fund(XRP(10000), "david", "eric", "frank", "greg");
        env(signers("alice", 2, { { "bob", 1 },   { "carol", 1 } }));
        env(signers("bob",   1, { { "david", 1 }, { "eric", 1 } }));
        env(signers("carol", 1, { { "frank", 1 }, { "greg", 1 } }));

        env(noop("alice"), msig2(
        { { "bob", "david" } }),                                ter(tefBAD_QUORUM));
        
        env(noop("alice"), msig2(
        { { "bob", "david" }, { "bob", "eric" } }),             ter(tefBAD_QUORUM));

        env(noop("alice"), msig2(
        { { "carol", "frank" } }),                              ter(tefBAD_QUORUM));
        
        env(noop("alice"), msig2(
        { { "carol", "frank" }, { "carol", "greg" } }),         ter(tefBAD_QUORUM));

        env(noop("alice"), msig2(
        { { "bob", "david" }, { "carol", "frank" } }));

        env(noop("alice"), msig2(
        { { "bob", "david" }, { "bob", "eric" },
          { "carol", "frank" }, { "carol", "greg" } }));
    }

    void
    testTicket()
    {
        using namespace jtx;
        // create syntax
        ticket::create("alice", "bob");
        ticket::create("alice", 60);
        ticket::create("alice", "bob", 60);
        ticket::create("alice", 60, "bob");

        {
            Env env(*this);
            env.fund(XRP(10000), "alice");
            env(noop("alice"),                  require(owners("alice", 0), tickets("alice", 0)));
            env(ticket::create("alice"),        require(owners("alice", 1), tickets("alice", 1)));
            env(ticket::create("alice"),        require(owners("alice", 2), tickets("alice", 2)));
        }

        Env env(*this);
    }

    void
    run()
    {
        // Hack to silence logging
        deprecatedLogs().severity(
            beast::Journal::Severity::kNone);

        testAccount();
        testAmount();
        testEnv();
        testRequire();
        testKeyType();
        testPayments();

        testMultiSign();
        testMultiSign2();
        testTicket();
    }
};

BEAST_DEFINE_TESTSUITE(Env,app,ripple)

} // test
} // ripple
