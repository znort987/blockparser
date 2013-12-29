blockparser peercoin fork
===========

    Credits:
    --------

        Written by znort987@yahoo.com (1ZnortsoStC1zSTXbW6CUtkvqew8czMMG)
        Adapted to peercoin by snakie@yahoo.com (PGiNfS4KTmb7W9GDxrA54tYTRhmSK36Pyj)

    What:
    -----

        A fairly fast, quick and dirty peercoin whole blockchain parser.

    Why:
    ----

        . Few dependencies: openssl-dev, boost

        . Very quickly extract information from the entire blockchain.

        . Code is simple and helps to understand how the data structures underlying bitcoin and peercoin work.

    Build it:
    ---------

        . Turn on your x86-64 linux machine:

        . Make sure you have an up to date peercoin client blockchain in ~/.ppcoin

        . Install these things: 
        
            libssl-dev build-essential g++-4.4 libboost-all-dev libsparsehash-dev git-core perl
            
            If on Ubuntu run:
                sudo apt-get install libssl-dev build-essential g++-4.4 libboost-all-dev \
                libsparsehash-dev git-core perl

        . Run this:

            git clone git://github.com/snakie/blockparser.git
            cd blockparser
            make
            ./parser

    Try it:
    -------

        . Show a list of commands

            ./parser help

        . Compute simple blockchain stats, full chain parse (0.173 s)

            ./parser stats

        . Extract all transactions for popular address P99kCfbcBjAmgZiwowLWH8sVf1wsdTeLNb (1.16 s)

            ./parser transactions P99kCfbcBjAmgZiwowLWH8sVf1wsdTeLNb

        . Compute the closure of an address, that is the list of addresses that provably belong to 
          the same person (1.665 s):

            ./parser closure P99kCfbcBjAmgZiwowLWH8sVf1wsdTeLNb

        . Compute and print the balance for all keys ever used in a TX since the beginning of 
          time (1.07 s):

            ./parser balances >balances.txt

        . See how much a random transaction tainted other TX in the chain

            ./parser taint >taint.txt

        . See all the block rewards and fees:

            ./parser rewards >rewards.txt

        . See a greatly detailed dump of a random proof of stake generation transaction

            ./parser show

    Caveats:
    --------

        . znort987 indicated this would not likely compile on anything but x86-84 ubuntu with
          with at least GCC >= 4.4. I managed to get it to compile on scientific linux with 
          GCC 4.4.6 with a few code tweaks.

        . It used to require alot of RAM with bitcoin, but peercoin has a relatively small 
          blockchain so this doesn't appear to be as big of a problem as of now. 

        . The code isn't particularly clean or well architected. It was just a quick way for 
          znort987 to learn about bitcoin and snakie for peercoin. There isnt much in the way 
          of comments either.

        . OTOH, it is fairly simple, short, and efficient. If you want to understand how the 
          blockchain data structure works, the code in parser.cpp is a solid way to start.

        . There are probably tons of peercoin bugs, but so far dumpTX, closure, balances, taint,
          pristine, rewards and transactions functions all seem to work well. 

    Hacking the code:
    -----------------

        . parser.cpp contains a generic parser that mmaps the blockchain, parses it and calls
          "user-defined" callbacks as it hits interesting bits of information.

        . util.cpp contains a grab-bag of useful bitcoin/peercoin related routines. 
          Interesting examples include:

            showScript
            getBaseReward
            solveOutputScript
            decompressPublicKey

        . cb/allBalances.cpp    :   code to all balance of all addresses.
        . cb/closure.cpp        :   code to compute the transitive closure of an address
        . cb/dumpTX.cpp         :   code to display a transaction in very great detail. 
        . cb/help.cpp           :   code to dump detailed help for all other commands
        . cb/pristine.cpp       :   code to show all "pristine" (i.e. unspent) blocks
        . cb/rewards.cpp        :   code to show all block rewards (including fees)
        . cb/simpleStats.cpp    :   code to compute simple stats.*
        . cb/sql.cpp            :   code to product an SQL dump of the blockchain*
        . cb/taint.cpp          :   code to compute the taint from a given TX to all TXs.
        . cb/transactions.cpp   :   code to extract all transactions pertaining to an address.
        
        * untested on Peercoin

        . You can very easily add your own custom command. You can use the existing callbacks in
          directory ./cb/ as a template to build your own:

                cp cb/allBalances.cpp cb/myExtractor.cpp
                Add to Makefile
                Hack away
                Recompile
                Run

        . You can also read the file callback.h (the base class from which you derive to implement 
          your own new commands). It has been heavily commented and should provide a good basis to 
          pick what to overload to achieve your goal.

        . The code makes heavy use of the google dense hash maps. You can switch it to use sparse 
          hash maps (see util.h, search for: DENSE, undef it). Sparse hash maps are slower but 
          save quite a bit of RAM.

    Observations Peercoin vs Bitcoin:
    ---------------------------------
        
        . The Peercoin magic number is 0xe5e9e8e6.
        . The Peercoin address prefix (version number) is 55 in decimal or 0x37 in hex. 
        . Peercoin has 2 less decimal places then bitcoin. This can be found by comparing the 
          BitcoinUnits::decimals functions in src/qt/bitcoinunits.cpp of peercoin and bitcoin.
        . Proof of stake blocks are marked with an empty (0 byte) output script in output[0] 
          of the coinbase transaction. The next transaction (always the second transaction) in 
          the block will contain the proof of stake input and reward. There are a few other rules 
          that can be found in the IsCoinStake() function in src/main.h of peercoin.  
        . Transaction have an additional timestamp as compared to bitcoin.
        . Proof of stake minting uses this additional timestamp as part of the hashed data to help 
          avoid pre-computation of proof of stake blocks. 
        . Proof of Stake and Proof of Work both have seperate difficulties.
        . All fees are destroyed, no part of the fee go to miners.


    License:
    --------

        Code is in the public domain.
        The Software shall be used for Good, not Evil.

