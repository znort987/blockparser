#ifndef __CALLBACK_H__
    #define __CALLBACK_H__

    #include <vector>
    #include <common.h>
    #include <option.h>

    struct Block;
    struct Callback
    {
        Callback();
        virtual const char                *name() const = 0;
        virtual const option::Descriptor *usage() const = 0;
        virtual void                    aliases(std::vector<const char *> &v) {          }

        virtual int          init(int argc, char *argv[]               ) { return 0;     }
        virtual bool   needTXHash(                                     ) { return false; }

        virtual void     startMap(const uint8_t *p                     ) {               }
        virtual void       endMap(const uint8_t *p                     ) {               }
        virtual void   startBlock(const uint8_t *p                     ) {               }
        virtual void     endBlock(const uint8_t *p                     ) {               }

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

        static void showAllHelps();
        static Callback *find(const char *name);
    };

#endif // __CALLBACK_H__

