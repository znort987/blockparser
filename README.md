blockparser
===========

    Credits:
    --------

        Written by znort987@yahoo.com
        If you find this useful: 1ZnortsoStC1zSTXbW6CUtkvqew8czMMG

    What:
    -----

        A fast, quick and dirty bitcoin blockchain parser.

    Why:
    ----

        . Few dependencies: openssl-dev, boost

        . Very quickly extract information from the entire blockchain.

        . Code is simple and helps to understand how the data structure underlying bitcoin works.

    Build it:
    ---------

        . Turn your x86-64 Ubuntu box on

        . Make sure you have an up to date satoshi client blockchain in ~/.bitcoin

        . Run this:

            sudo apt-get install libssl-dev build-essential g++-4.4 libboost-all-dev libsparsehash-dev git-core perl
            git clone git://github.com/znort987/blockparser.git
            cd blockparser
            make

    Try it:
    -------

        . Compute simple blockchain stats, full chain parse (< 1 second)

            ./parser simpleStats

        . Extract all transactions for popular address 1dice6wBxymYi3t94heUAG6MpG5eceLG1 (20 seconds)

            ./parser transactions 06f1b66fa14429389cbffa656966993eab656f37

        . Compute the closure of an address, that is the list of addresses that provably belong to the same person (20 seconds):

            ./parser closure 06f1b66fa14429389cbffa656966993eab656f37

        . Compute and print the balance for all keys ever used in a TX since the beginning of time (30 seconds):

            ./parser allBalances >allBalances.txt

        . See how much of the BTC 10K pizza tainted each of the TX in the chain

            ./parser taint >pizzaTaint.txt

        . See all the block rewards and fees:

            ./parser rewards >rewards.txt

    Caveats:
    --------

        . You need an x86-84 ubuntu box and a recent version of GCC(>=4.4), recent versions of boost
          and openssl-dev. The whole thing is very unlikely to work or even compile on anything else.

        . It needs quite a bit of RAM to work. Never exactly measured how much, but the hash maps will
          grow quite fat. I might switch them to something different that spills over to disk at some
          point. For now: it works fine with 8 Gigs.

        . The code isnt particularly clean or well architected. It was just a quick way for me to learn
          about bitcoin. There isnt much in the way of comments.

        . OTOH, it is fairly simple, short, and efficient. If you want to understand how the blockchain
          data structure works, the code in parser.cpp is a fairly good way to start.

    Hacking the code:
    -----------------

        . parser.cpp contains a generic parser that mmaps the blockchain, parses it and calls "user-defined"
          callbacks as it hits interesting bits of information.

        . util.cpp contains a grab-bag of useful bitcoin related routines. Interesting examples include:

            showScript
            solveOutputScript
            decompressPublicKey

        . cb/allBalances.cpp    :   code to all balance of all addresses.
        . cb/closure.cpp        :   code to compute the transitive closure of an address.
        . cb/help.cpp           :   code to dump detailed help for all other commands
        . cb/pristine.cpp       :   code to show all "pristine" (i.e. unspent) blocks
        . cb/rewards.cpp        :   code to show all block rewards (including fees)
        . cb/simpleStats.cpp    :   code to compute simple stats.
        . cb/sql.cpp            :   code to product an SQL dump of the blockchain
        . cb/taint.cpp          :   code to compute the taint from a given TX to all TXs.
        . cb/transactions.cpp   :   code to extract all transactions pertaining to an address.

        . You can add your own custom callback. You can use the existing callbacks in
          directory ./cb/ to build your own:

                cp cb/allBalances.cpp cb/myExtractor.cpp
                Add to Makefile
                Hack away
                Recompile

        . The code makes heavy use of the google dense hash maps. You can switch it to use sparse hash
          maps (see util.h, search for: DENSE, undef it). Sparse hash maps are slower but save quite a
          bit of RAM.

    License:
    --------

        Code is in the public domain.

