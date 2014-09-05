# blockparser

A fairly fast, quick and dirty bitcoin blockchain parser.

## Quickstart

The blockparser can be run as a docker container or built from source. The docker approach avoid messing with compilation but requires that you have docker installed.

**Note** Both the docker and manual compilation approach require that you first run the [reference satoshi client](https://bitcoin.org/en/downloa) to download the blk###.dat files for the parser. The parser does not download blocks, it only parses downloaded block files and will expect to find them in the default location.

### Docker

The blockparser is available as a [docker](https://www.docker.com/) [container](https://registry.hub.docker.com/u/defunctzombie/blockparser/) so you can run it immediately on a system with [docker installed](http://docs.docker.com/installation/). It helps if you are slightly familiar with docker before trying this. This approach is recommended if you want to parse the blockchain on AWS or similar compute infrastructure without manual compilation.

```
docker run -i -t -v /path/to/.bitcoin:/home/bitcoin/.bitcoin defunctzombie/blockparser help
```

### From Source

Instructions to build blockparser from source on an *recent* ubuntu or debian box. If you don't want to mess with this approach, I suggest learning about docker.

```
sudo apt-get install libssl-dev cmake clang-3.4 libboost-graph-dev libsparsehash-dev git
git clone git://github.com/defunctzombie/blockparser.git
cd blockparser
mkdir build && cd build && cmake .. && make
```

## Why

The blockchain is massive and extracing certain information is only practical if done against local copies of the block files.

The code is simple and helps to understand how the data structure underlying bitcoin works.

## Commands

The following commands are currently supported.

* Compute simple blockchain stats, full chain parse (< 1 second)
```
./parser simpleStats
```

* Extract all transactions for popular address 1dice6wBxymYi3t94heUAG6MpG5eceLG1 (20 seconds)
```
./parser transactions 06f1b66fa14429389cbffa656966993eab656f37
```

* Compute the closure of an address, that is the list of addresses that provably belong to the same person (20 seconds):
```
./parser closure 06f1b66fa14429389cbffa656966993eab656f37
```

* Compute and print the balance for all keys ever used in a TX since the beginning of time (30 seconds):
```
./parser allBalances >allBalances.txt
```

* See how much of the BTC 10K pizza tainted each of the TX in the chain
```
./parser taint >pizzaTaint.txt
```

* See all the block rewards and fees:
```
./parser rewards >rewards.txt
```

* See a greatly detailed dump of the pizza transaction

```
  ./parser show
```

## Caveats

* You need an x86-84 ubuntu box and a recent version of GCC(>=4.4), recent versions of boost
  and openssl-dev. The whole thing is very unlikely to work or even compile on anything else.

* It needs quite a bit of RAM to work. Never exactly measured how much, but the hash maps will
  grow quite fat. I might switch them to something different that spills over to disk at some
  point. For now: it works fine with 8 Gigs.

* The code isn't particularly clean or well architected. It was just a quick way for me to learn
  about bitcoin. There isnt much in the way of comments either.

* OTOH, it is fairly simple, short, and efficient. If you want to understand how the blockchain
  data structure works, the code in parser.cpp is a solid way to start.

## Hacking the code

* parser.cpp contains a generic parser that mmaps the blockchain, parses it and calls
  "user-defined" callbacks as it hits interesting bits of information.

* util.cpp contains a grab-bag of useful bitcoin related routines. Interesting examples include:

    showScript
    getBaseReward
    solveOutputScript
    decompressPublicKey

cb/allBalances.cpp    :   code to all balance of all addresses.  
cb/closure.cpp        :   code to compute the transitive closure of an address  
cb/dumpTX.cpp         :   code to display a transaction in very great detail  
cb/help.cpp           :   code to dump detailed help for all other commands  
cb/pristine.cpp       :   code to show all "pristine" (i.e. unspent) blocks  
cb/rewards.cpp        :   code to show all block rewards (including fees)  
cb/simpleStats.cpp    :   code to compute simple stats.  
cb/sql.cpp            :   code to product an SQL dump of the blockchain  
cb/taint.cpp          :   code to compute the taint from a given TX to all TXs.  
cb/transactions.cpp   :   code to extract all transactions pertaining to an address.  

* You can very easily add your own custom command. You can use the existing callbacks in
  directory ./cb/ as a template to build your own:

```
cp cb/allBalances.cpp cb/myExtractor.cpp
Add to CMakeLists.txt
Hack away
Recompile
Run
```

* You can also read the file callback.h (the base class from which you derive to implement your
  own new commands). It has been heavily commented and should provide a good basis to pick what
  to overload to achieve your goal.

* The code makes heavy use of the google dense hash maps. You can switch it to use sparse hash
  maps (see util.h, search for: DENSE, undef it). Sparse hash maps are slower but save quite a
  bit of RAM.

## Credits

Written by znort987@yahoo.com  
If you find this useful, donate **1ZnortsoStC1zSTXbW6CUtkvqew8czMMG**

## License

Code is in the public domain.

