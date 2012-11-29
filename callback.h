#ifndef __CALLBACK_H__
    #define __CALLBACK_H__

    #include <vector>
    #include <common.h>
    #include <option.h>

    struct Block;
    struct Callback
    {
        Callback();
        virtual const char                   *name() const = 0;
        virtual const optparse::OptionParser *optionParser() const = 0;
        virtual bool                         needTXHash() const                          { return false; }
        virtual void                         aliases(std::vector<const char *> &v) const {               }

        virtual int          init(int argc, const char *argv[]         ) { return 0;     }

        // Callback for first, shallow parse -- all blocks are seen, including orphaned ones but aren't parsed
        virtual void     startMap(const uint8_t *p                     ) {               }
        virtual void       endMap(const uint8_t *p                     ) {               }
        virtual void   startBlock(const uint8_t *p                     ) {               }
        virtual void     endBlock(const uint8_t *p                     ) {               }

        // Callback for second, deep parse -- only valid blocks are seen, and are parsed in details
        virtual void        start(  const Block *s, const Block *e     ) {               }
        virtual void      startTX(const uint8_t *p, const uint8_t *hash) {               }
        virtual void        endTX(const uint8_t *p                     ) {               }
        virtual void  startInputs(const uint8_t *p                     ) {               }
        virtual void    endInputs(const uint8_t *p                     ) {               }
        virtual void   startInput(const uint8_t *p                     ) {               }
        virtual void     endInput(const uint8_t *p                     ) {               }
        virtual void startOutputs(const uint8_t *p                     ) {               }
        virtual void   endOutputs(const uint8_t *p                     ) {               }
        virtual void  startOutput(const uint8_t *p                     ) {               }
        virtual void   startBlock(  const Block *b                     ) {               }
        virtual void     endBlock(  const Block *b                     ) {               }
        virtual void       wrapup(                                     ) {               }

        virtual void endOutput(
            const uint8_t *p,
            uint64_t      value,
            const uint8_t *txHash,
            uint64_t      outputIndex,
            const uint8_t *outputScript,
            uint64_t      outputScriptSize
        )
        {
        }

        virtual void edge(
            uint64_t      value,
            const uint8_t *upTXHash,
            uint64_t      outputIndex,
            const uint8_t *outputScript,
            uint64_t      outputScriptSize,
            const uint8_t *downTXHash,
            uint64_t      inputIndex,
            const uint8_t *inputScript,
            uint64_t      inputScriptSize
        )
        {
        }

        static Callback *find(const char *name, bool printList=false);
        static void showAllHelps(bool longHelp);
    };

#endif // __CALLBACK_H__

