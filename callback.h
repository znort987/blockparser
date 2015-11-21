#ifndef __CALLBACK_H__
    #define __CALLBACK_H__

    struct Block;
    #include <vector>
    #include <common.h>
    #include <option.h>

    // Derive from this if you want to add a new command
    struct Callback
    {
        // Housekeeping
        Callback();
        typedef optparse::OptionParser Parser;
        static void showAllHelps(bool longHelp);
        static void showHelpFor(const char*, bool longHelp);
        static Callback *find(const char *name, bool printList=false);

        // Naming, option parsing, construction, etc ...
        virtual const char           *name(                            ) const = 0;              // Main name for callback
        virtual const Parser *optionParser(                            ) const = 0;              // Option parser object for callback
        virtual void               aliases(std::vector<const char *> &v) const {               } // Alternate names for callback
        virtual int                   init(int argc, const char *argv[])       { return 0;     } // Called after callback construction, with command line arguments
        virtual bool          needUpstream(                            ) const { return false; } // Overload if you need parser to provide upstream to each input (slower + memory hungry)

        // Callback for first, shallow parse -- all blocks are seen, including orphaned ones but aren't parsed
        virtual void startBlockFile(const uint8_t *p                   )       {               }  // Called when a blockchain file is mapped into memory
        virtual void   endBlockFile(const uint8_t *p                   )       {               }  // Called when a blockchain file is unmapped from memory
        virtual void     startBlock(const uint8_t *p                   )       {               }  // Called when a block is encountered during first pass
        virtual void       endBlock(const uint8_t *p                   )       {               }  // Called when an end of block is encountered during first pass

        // Callback for second, deep parse -- only valid blocks are seen, and are parsed in details
        virtual void        start(  const Block *s, const Block *e     )       {               }  // Called when the second parse of the full chain starts
        virtual void     startTXs(const uint8_t *p                     )       {               }  // Called when start list of TX is encountered
        virtual void       endTXs(const uint8_t *p                     )       {               }  // Called when end list of TX is encountered
        virtual void      startTX(const uint8_t *p, const uint8_t *hash)       {               }  // Called when a new TX is encountered
        virtual void        endTX(const uint8_t *p                     )       {               }  // Called when an end of TX is encountered
        virtual void  startInputs(const uint8_t *p                     )       {               }  // Called when the start of a TX's input array is encountered
        virtual void    endInputs(const uint8_t *p                     )       {               }  // Called when the end of a TX's input array is encountered
        virtual void   startInput(const uint8_t *p                     )       {               }  // Called when a TX input is encountered
        virtual void     endInput(const uint8_t *p                     )       {               }  // Called when at the end of a TX input
        virtual void startOutputs(const uint8_t *p                     )       {               }  // Called when the start of a TX's output array is encountered
        virtual void   endOutputs(const uint8_t *p                     )       {               }  // Called when the end of a TX's output array is encountered
        virtual void  startOutput(const uint8_t *p                     )       {               }  // Called when a TX output is encountered
        virtual void   startBlock(  const Block *b, uint64_t chainSize )       {               }  // Called when a new block is encountered
        virtual void     endBlock(  const Block *b                     )       {               }  // Called when an end of block is encountered
        virtual void      startLC(                                     )       {               }  // Called when longest chain parse starts
        virtual void       wrapup(                                     )       {               }  // Called when the whole chain has been parsed
        virtual bool         done(                                     )       { return false; }  // Called after each TX to check if callback is done

        // Called when an output has been fully parsed
        virtual void endOutput(
            const uint8_t *p,                   // Pointer to TX output raw data
            uint64_t      value,                // Number of satoshis on this output
            const uint8_t *txHash,              // sha256 of the current transaction
            uint64_t      outputIndex,          // Index of this output in the current transaction
            const uint8_t *outputScript,        // Raw script (challenge to would-be spender) carried by this output
            uint64_t      outputScriptSize      // Byte size of raw script
        ) {
        }

        // Called exactly like startInput, but with a much richer context
        virtual void edge(
            uint64_t      value,                // Number of satoshis coming in on this input from upstream transaction
            const uint8_t *upTXHash,            // sha256 of upstream transaction
            uint64_t      outputIndex,          // Index of output in upstream transaction
            const uint8_t *outputScript,        // Raw script (challenge to spender) carried by output in upstream transaction
            uint64_t      outputScriptSize,     // Byte size of script carried by output in upstream transaction
            const uint8_t *downTXHash,          // sha256 of current (downstream) transaction
            uint64_t      inputIndex,           // Index of input in downstream transaction
            const uint8_t *inputScript,         // Raw script (answer to challenge) carried by input in downstream transaction
            uint64_t      inputScriptSize       // Byte size of script carried by input in downstream transaction
        ) {
        }
    };

#endif // __CALLBACK_H__

