blockparser
===========

    Who wrote it ?
    --------------

        Author:

            znort987@yahoo.com

        Tip here if you find it useful:

            1ZnortsoStC1zSTXbW6CUtkvqew8czMMG

        I've also been cherry-picking changes I found useful from various github forks.
        Credits for these:

             git log | grep Author | grep -iv Znort

    Canonical source code repo:
    ---------------------------

        git clone github.com:znort987/blockparser.git

    License:
    --------

        Code is in the public domain.

    What is it ?
    ------------

        A barebone C++ block parser that parses the entire block chain from scratch
        to extract various types of information from it.

        The code chews "linearly" through the block chain and calls "user-defined"
        callbacks when it hits on certain "events" in the chain. Here:

            "events" essentially means that the parser is starting to assemble a new
            blockchain data structure (a block, a tx, an input, etc ...), or that the
            parser has just completed a data structure, in which case it will usually
            run the callback with the completed data structure. The blockchain data
            structure level of granularity at which these "events" happen is somewhat
            arbitrary.  For example you won't get called every time a new byte is seen.

            "user-defined" means that if you want to extract new types of information
            from the chain, you have to add your own C++ piece of code to those already
            in directory "cb". Your C++ code will get called by the main parser at
            "events" of your choosing.

            "linearly" is a bit of an abuse because the parser code often has to jump
            back to previously seen parts of the blockchain to provide user callbacks
            with fully complete data structures. The parser code also has to walk the
            blockchain a few times to compute the longest (valid) chain. But the user
            callbacks get a fairly linear view of it all.


        Blockparser was designed for bitcoin but works on most altcoins that were
        derived from the bitcoin code base.

    What it is not:
    ---------------

        Blockparser is *not* a verifier. It assumes a clean blockchain, as typically
        constructed and verified by the bitcoin core client. blockparser does not
        perform any kind of verification and will likely crash if applied to an unclean
        chain.

        Blockparser is not very efficient if you want to perform repetitive tasks on
        thr block chain: the basic idea/premise of blockparser is that it's going to
        chew through the *entire* block chain, *every* time. Given the size of the
        blockchain these days, that's not something you want to do very 5 minutes.

        Blockparser is not lean and mean. It used to be, when the blockchain was still
        relatively small.  Now that we are inching towards the 100's of gigabytes, the
        very proposition that it has to chew through entire chain by design implies that
        it's going to take quite a while, whichever way you slice it. Also, the entire
        index is built on the fly and kept in RAM. At current sizes, this is not a very
        smart choice. This might get addressed in the near future.

    Why write this ?
    ----------------

        It started as an exercise for me to get a "close to the metal" understanding of
        how bitcoin works. The quality and state of the original bitcoin codebase made
        this damn near impossible (it's clear to me satosh, albeit clearly a genius, was
        not a professional software engineer. Also, things have vastly improved since then).
        It then grew into a fun hobby project.

        The parser code is minimal and very easy to follow. If someone wants to quickly
        understand "for real" how the block chain is structured, it is a good place to
        start

        It has also slowly grown into an altcoin zoo. It is very far from being a
        compendium (there's so many of the darn things these days), but adding your
        fave alt is very easy.

        Talking about zoo, I've also started to track and document "weird" TXO's
        in the chain (comments, p2sh, multi-sigs, bugs, etc ...). Not a complete
        compendium yet, but getting there.

        A side goal was also to build something that can independently (as in : the
        codebase is *very* different from that of bitcoin core) verify some of the
        conclusions of other bitcoin implementations, such as how many coins are
        attached to an address.

        Another thing that blockparser is really nice for is to easily reconstruct
        "snapshots" of the state of the blockchain from a specific time (e.g. the -a
        option of the "allBalances" command).

    How do I build it ?
    -------------------

        You'll need a 64-bit Unix box (because of RAM consumption, blockparser won't work
        inside a 32bit address space).

        If you are unfortunate enough to still have to use windows, there is a port floating
        somehwere on github.

        I also have heard rumors of it working on OSX.

        You'll need a block chain somewhere on your hard drive. This is typically created
        by a statoshi bitcoin client such as this one: https://github.com/bitcoin/bitcoin.git

        Install dependencies:

            sudo apt-get install libssl-dev build-essential g++ libboost-all-dev libsparsehash-dev git-core perl

        Get the source:

            git clone git://github.com/znort987/blockparser.git

        Build it:

            cd blockparser
            make

    It crashes
    ----------

        At this point, blockparser uses a *lot* of memory (20+ Gig is typical). This
        can cause all sorts of woes on an under-dimensioned box, chief amongst which:

            - box goes into heavy swapping, and parser takes for ever to complete task

            - parser eats up all RAM and all SWAP and crashes. Here's a possible remedy:

                 http://askubuntu.com/questions/178712/how-to-increase-swap-space

    How does blockparser deal with multi-sig transactions ?
    --------------------------------------------------------

        AFAIK, there are two types of multi-sig transactions:

            1) Pay-to-script (which is in fact more general than multisig). This one is
               easy, because it pays to a hash, which can readily be converted to an
               address that starts with the character '3' instead of '1'

            2) Naked multi-sig transactions. These are harder, because the output of
               the transactions does not neatly map to a specific bitcoin address. I
               think I have found a neat work-around: I compute:

                     hash160(M, N, sortedListOfAddresses)

               which can now be properly mapped to a bitcoin address. To mark the fact
               that this addres is neither a "pay to script" (type '3') nor a
               "pay to pubkey or pubkeyhash" (type '1'), I prefix them with '4'

               Note : this may be worthy of an actual BIP. If someone writes one,
               I'll happily adjust the code.

               Note : this trick is only a blockparser thing. This means that these
               new address types starting with a '4' won't be recognized by other
               bitcoin implementations (such as blockchain.info)

    Examples
    --------

        . Show all supported commands

            ./parser help

        . Show help for a specific command

            ./parser allBalances --help

        . Compute simple blockchain stats

            ./parser simple

        . Extract all transactions for a very popular address 1dice6wBxymYi3t94heUAG6MpG5eceLG1

            ./parser transactions 06f1b66fa14429389cbffa656966993eab656f37

        . Compute the closure of an address, that is the list of addresses that very probably belong to the same person:

            ./parser closure 06f1b66fa14429389cbffa656966993eab656f37

        . Compute and print the balance for all keys ever used since the beginning of time:

            ./parser all >all.txt

        . See how much of the BTC 10K pizza tainted all the subsequent TX in the chain
          (chances are you have some dust coming from that famous TX lingering on one
          of your addresses)

            ./parser taint >pizzaTaint.txt

        . See all the block rewards and fees:

            ./parser rewards >rewards.txt

        . See a greatly detailed dump of the famous pizza transaction

            ./parser show

        . Track all mined blocks with unspent reward:

            ./parser pristine

        . Show the first valid "pay to script hash (P2SH)" transaction in the chain:

            ./parser showtx 9c08a4d78931342b37fd5f72900fb9983087e6f46c4a097d8a1f52c74e28eaf6

        . Show the first valid naked multi-sig transaction in the chain (it's a 1 Of 2 multi-sig)

            ./parser showtx 60a20bd93aa49ab4b28d514ec10b06e1829ce6818ec06cd3aabd013ebcdc4bb1

    NOTE: the general syntax is:

        ./parser <command> <option> <option> ... <arg> <arg> ...


    NOTE: use "parser help <command>" or "parser <command> --help" to get detailed
          help for a specific command.

    NOTE: <command> may have multiple aliases and can also be abbreviated. For
          example, "parser tx", "parser tr", and "parser transactions" are equivalent.

    NOTE: whenever specifying a list of things (e.g. a list of addresses), you can
          instead enter "file:list.txt" and the list will be read from the file.

    NOTE: whenever specifying a list file, you can use "file:-" and blockparser
          will read the list directly from stdin.


    Caveats:
    --------

        . You will need an x86-84 ubuntu box and a recent version of GCC(>=4.4), a recent version of
          boost and openssl-dev. You may be able to compile on other platforms, but the code wasn't
          really designed for those.

        . As of this writing, it needs a log of RAM to work, typically upwards of 25Gigs. I will switch
          to an on-disk hash table at some point, but for now you'll just need that if you ever hope to
          see it finish in a reasonable amount of time (or at all if your swap space is too small).

        . The code could be cleaner and better architected. It was just a quick and dirty way for me
          to learn about bitcoin. There really isn't much in the way of comments either :D

        . OTOH, it is fairly simple, short. If you want to understand how the blockchain data structures
          work, the code in parser.cpp is a solid way to start.

    Hacking the code:
    -----------------

        . parser.cpp contains the generic parser that reads the blockchain, parses it and calls
          "user-defined" callbacks as it hits interesting bits of information. It typically calls
          out when it begins reading finishes assembling a data structure.

        . util.cpp contains a grab-bag of useful bitcoin related routines. Interesting examples include:

            showScript
            getBaseReward
            solveOutputScript
            decompressPublicKey

        . blockparser comes with a bunch of interesting "user callbacks".

            . cb/allBalances.cpp    :   code to all balance of all addresses.
            . cb/closure.cpp        :   code to compute the transitive closure of an address
            . cb/dumpTX.cpp         :   code to display a transaction in very great detail
            . cb/help.cpp           :   code to dump detailed help for all other commands
            . cb/pristine.cpp       :   code to show all "pristine" (i.e. unspent) blocks
            . cb/rewards.cpp        :   code to show all block rewards (including fees)
            . cb/simpleStats.cpp    :   code to compute simple stats.
            . cb/sql.cpp            :   code to product an SQL dump of the blockchain
            . cb/taint.cpp          :   code to compute the taint from a given TX to all TXs.
            . cb/transactions.cpp   :   code to extract all transactions pertaining to an address.


        . You can very easily add your own custom command. You can use the existing callbacks in
          directory ./cb/ as a template to build your own:

                cp cb/allBalances.cpp cb/myExtractor.cpp
                Add to Makefile
                Hack away
                Recompile
                Run

        . You can also read the file callback.h (the base class from which you derive to implement your
          own new commands). It has been heavily commented and should provide a good basis to pick what
          to overload to achieve your goal.

        . The code makes heavy use of the google dense hash maps. You can switch it to use sparse hash
          maps (see Makefile, search for: DENSE, undef it). Sparse hash maps are slower but save quite a
          bit of RAM.

