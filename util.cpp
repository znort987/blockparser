
#include <util.h>
#include <alloca.h>
#include <common.h>
#include <errlog.h>
#include <rmd160.h>
#include <sha256.h>
#include <opcodes.h>

#include <string>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

const uint8_t hexDigits[] = "0123456789abcdef";
const uint8_t b58Digits[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

template<> uint8_t *PagedAllocator<Block>::pool = 0;
template<> uint8_t *PagedAllocator<Block>::poolEnd = 0;

template<> uint8_t *PagedAllocator<uint256_t>::pool = 0;
template<> uint8_t *PagedAllocator<uint256_t>::poolEnd = 0;

template<> uint8_t *PagedAllocator<uint160_t>::pool = 0;
template<> uint8_t *PagedAllocator<uint160_t>::poolEnd = 0;

template<> uint8_t *PagedAllocator<Chunk>::pool = 0;
template<> uint8_t *PagedAllocator<Chunk>::poolEnd = 0;

double usecs() {
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_usec + 1000000*((uint64_t)t.tv_sec);
}

void toHex(
          uint8_t *dst,     // 2*size +1
    const uint8_t *src,     // size
    size_t        size,
    bool          rev
)
{
    int incr = 1;
    const uint8_t *p = src;
    const uint8_t *e = size + src;
    if(rev)
    {
        p = e-1;
        e = src-1;
        incr = -1;
    }
    
    while(likely(p!=e))
    {
        uint8_t c = p[0];
        dst[0] = hexDigits[c>>4];
        dst[1] = hexDigits[c&0xF];
        p += incr;
        dst += 2;
    }
    dst[0] = 0;
}

void showHex(
    const uint8_t *p,
    size_t        size,
    bool          rev
)
{
    uint8_t* buf = (uint8_t*)alloca(2*size + 1);
    toHex(buf, p, size, rev);
    printf("%s", buf);
}

uint8_t fromHexDigit(
    uint8_t h,
    bool abortOnErr
)
{
    if(likely('0'<=h && h<='9')) return      (h - '0');
    if(likely('a'<=h && h<='f')) return 10 + (h - 'a');
    if(likely('A'<=h && h<='F')) return 10 + (h - 'A');
    if(abortOnErr) errFatal("incorrect hex digit %c", h);
    return 0xFF;
}

bool fromHex(
          uint8_t *dst,
    const uint8_t *src,
    size_t        dstSize,
    bool          rev,
    bool          abortOnErr
)
{
    int incr = 2;
    uint8_t *end = dstSize + dst;
    if(rev)
    {
        src += 2*(dstSize-1);
        incr = -2;
    }

    while(likely(dst<end))
    {
        uint8_t hi = fromHexDigit(src[0], abortOnErr);
        if(unlikely(0xFF==hi)) return false;

        uint8_t lo = fromHexDigit(src[1], abortOnErr);
        if(unlikely(0xFF==lo)) return false;

        *(dst++) = (hi<<4) + lo;
        src += incr;
    }

    return true;
}

void showScript(
    const uint8_t *p,
    size_t        scriptSize,
    const char    *header,
    const char    *indent
)
{
    bool first = true;
    const uint8_t *e = scriptSize + p;
    indent = indent ? indent : "";
    while(likely(p<e)) {
        LOAD(uint8_t, c, p);
        bool isImmediate = (0<c && c<79) ;
        if(!isImmediate) {
            printf(
                "    %s0x%02X %s%s\n",
                indent,
                c,
                getOpcodeName(c),
                (first && header) ? header : ""
            );
        }
        else
        {
            uint64_t dataSize = 0;
                 if(likely(c<=75)) {                       dataSize = c; }
            else if(likely(76==c)) { LOAD( uint8_t, v, p); dataSize = v; }
            else if(likely(77==c)) { LOAD(uint16_t, v, p); dataSize = v; }
            else if(likely(78==c)) { LOAD(uint32_t, v, p); dataSize = v; }
            printf("         %sOP_PUSHDATA(%" PRIu64 ", 0x", indent, dataSize);
            showHex(p, dataSize, false);

            printf(
                ")%s\n",
                (first && header) ? header : ""
            );
            p += dataSize;
        }
        first = false;
    }
}

bool compressPublicKey(
          uint8_t *result,          // 33 bytes
    const uint8_t *decompressedKey  // 65 bytes
)
{
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if(!key) {
        errFatal("EC_KEY_new_by_curve_name failed");
        return false;
    }

    EC_KEY *r = o2i_ECPublicKey(&key, &decompressedKey, 65);
    if(!r) {
        //warning("o2i_ECPublicKey failed");
        EC_KEY_free(key);
        return false;
    }

    EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    size_t size = i2o_ECPublicKey(key, &result);
    EC_KEY_free(key);

    if(33!=size) {
        errFatal("i2o_ECPublicKey failed");
        return false;
    }

    return true;
}

bool decompressPublicKey(
          uint8_t *result,          // 65 bytes
    const uint8_t *compressedKey    // 33 bytes
)
{
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if(!key) {
        errFatal("EC_KEY_new_by_curve_name failed");
        return false;
    }

    EC_KEY *r = o2i_ECPublicKey(&key, &compressedKey, 33);
    if(!r) {
        //warning("o2i_ECPublicKey failed");
        EC_KEY_free(key);
        return false;
    }

    EC_KEY_set_conv_form(key, POINT_CONVERSION_UNCOMPRESSED);
    size_t size = i2o_ECPublicKey(key, &result);
    EC_KEY_free(key);

    if(65!=size) {
        errFatal("i2o_ECPublicKey failed");
        return false;
    }

    return true;
}

int solveOutputScript(
          uint8_t *pubKeyHash,
    const uint8_t *script,
    uint64_t      scriptSize,
    uint8_t       *type
)
{
    type[0] = 0;

    // The most common output script type, pays to hash160(pubKey)
    if(
        likely(
            0x76==script[0]              &&  // OP_DUP
            0xA9==script[1]              &&  // OP_HASH160
              20==script[2]              &&  // OP_PUSHDATA(20)
            0x88==script[scriptSize-2]   &&  // OP_EQUALVERIFY
            0xAC==script[scriptSize-1]   &&  // OP_CHECKSIG
              25==scriptSize
        )
    )
    {
        memcpy(pubKeyHash, 3+script, kRIPEMD160ByteSize);
        return 0;
    }

    // Output script commonly found in block reward TX, pays to explicit pubKey
    if(
        likely(
              65==script[0]             &&  // OP_PUSHDATA(65)
            0xAC==script[scriptSize-1]  &&  // OP_CHECKSIG
              67==scriptSize
        )
    )
    {
        uint256_t sha;
        sha256(sha.v, 1+script, 65);
        rmd160(pubKeyHash, sha.v, kSHA256ByteSize);
        return 1;
    }

    // Unusual output script, pays to explicit compressed pubKeys
    if(
        likely(
              33==script[0]            &&  // OP_PUSHDATA(33)
            0xAC==script[scriptSize-1] &&  // OP_CHECKSIG
              35==scriptSize
        )
    )
    {
        //uint8_t pubKey[65];
        //bool ok = decompressPublicKey(pubKey, 1+script);
        //if(!ok) return -3;

        uint256_t sha;
        sha256(sha.v, 1+script, 33);
        rmd160(pubKeyHash, sha.v, kSHA256ByteSize);
        return 2;
    }

    // Recent output script type, pays to hash160(script)
    if(
        likely(
            0xA9==script[0]             &&  // OP_HASH160
              20==script[1]             &&  // OP_PUSHDATA(20)
            0x87==script[scriptSize-1]  &&  // OP_EQUAL
              23==scriptSize
        )
    )
    {
        memcpy(pubKeyHash, 2+script, kRIPEMD160ByteSize);
        type[0] = 'S';
        type[1] = 0;
        return 3;
    }

    // Broken output scripts that were created by p2pool for a while -- very likely lost coins
    if(
        0x73==script[0] && // OP_IFDUP
        0x63==script[1] && // OP_IF
        0x72==script[2] && // OP_2SWAP
        0x69==script[3] && // OP_VERIFY
        0x70==script[4] && // OP_2OVER
        0x74==script[5]    // OP_DEPTH
    )
        return -2;

#if 0

    // TODO : some scripts are solved by satoshi's client and not by the above. track them

    // Unknown output script type -- very likely lost coins, but hit the satoshi script solver to make sure
    int result = extractAddress(pubKeyHash, script, scriptSize);
    if(result) return -1;
    return 5;
    printf("EXOTIC OUTPUT SCRIPT:\n");
    showScript(script, scriptSize);
#endif
    return -1;
}

const uint8_t *loadKeyHash(
    const uint8_t *hexHash
)
{
    static bool loaded = false;
    static uint8_t hash[kRIPEMD160ByteSize];
    const char *someHexHash = "0568015a9facccfd09d70d409b6fc1a5546cecc6"; // 1VayNert3x1KzbpzMGt2qdqrAThiRovi8 deepbit's very large address

    if(unlikely(!loaded))
    {
        if(0==hexHash)
            hexHash = reinterpret_cast<const uint8_t *>(someHexHash);

        if((2*kRIPEMD160ByteSize)!=strlen((const char *)hexHash))
            errFatal("specified hash has wrong length");

        fromHex(hash, hexHash, sizeof(hash), false);
        loaded = true;
    }

    return hash;
}

uint8_t fromB58Digit(
    uint8_t digit,
       bool abortOnErr
)
{
    if('1'<=digit && digit<='9') return (digit - '1') +   0;
    if('A'<=digit && digit<='H') return (digit - 'A') +   9;
    if('J'<=digit && digit<='N') return (digit - 'J') +  17;
    if('P'<=digit && digit<='Z') return (digit - 'P') +  22;
    if('a'<=digit && digit<='k') return (digit - 'a') +  33;
    if('m'<=digit && digit<='z') return (digit - 'm') +  44;
    if(abortOnErr) errFatal("incorrect base58 digit %c", digit);
    return 0xff;
}

bool addrToHash160(
          uint8_t *hash160,
    const uint8_t *addr,
             bool checkHash,
             bool verbose
)
{
    static BIGNUM *sum = 0;
    static BN_CTX *ctx = 0;
    if(unlikely(!ctx)) {
        ctx = BN_CTX_new();
        BN_CTX_init(ctx);
        sum = BN_new();
    }

    BN_zero(sum);
    while(1) {
        uint8_t c = *(addr++);
        if(unlikely(0==c)) break;

        uint8_t dg = fromB58Digit(c);
        BN_mul_word(sum, 58);
        BN_add_word(sum, dg);
    }

    uint8_t buf[4 + 2 + kRIPEMD160ByteSize + 4];
    size_t size = BN_bn2mpi(sum, 0);
    if(sizeof(buf)<size) {
        warning(
            "BN_bn2mpi returned weird buffer size %d, expected %d\n",
            (int)size,
            (int)sizeof(buf)
        );
        return false;
    }

    BN_bn2mpi(sum, buf);

    uint32_t recordedSize = 
        (buf[0]<<24)    |
        (buf[1]<<16)    |
        (buf[2]<< 8)    |
        (buf[3]<< 0);
    if(size!=(4+recordedSize)) {
        warning(
            "BN_bn2mpi returned bignum size %d, expected %d\n",
            (int)recordedSize,
            (int)size-4
        );
        return false;
    }

    uint8_t *bigNumEnd;
    uint8_t *dataEnd = size + buf;
    uint8_t *bigNumStart = 4 + buf;
    uint8_t *checkSumStart = bigNumEnd = (-4 + dataEnd);
    while(0==bigNumStart[0] && bigNumStart<checkSumStart) ++bigNumStart;

    ptrdiff_t bigNumSize = bigNumEnd - bigNumStart;
    ptrdiff_t padSize = kRIPEMD160ByteSize - bigNumSize;
    if(0<padSize) {
        if(0<bigNumSize) memcpy(padSize + hash160, bigNumStart, bigNumSize);
        memset(hash160, 0, padSize);
    } else {
        memcpy(hash160, bigNumStart - padSize, kRIPEMD160ByteSize);
    }

    bool hashOK = true;
    if(checkHash) {

        uint8_t data[1+kRIPEMD160ByteSize];
        memcpy(1+data, hash160, kRIPEMD160ByteSize);

        #if defined(PROTOSHARES)
            uint8_t type = 56
        #endif

        #if defined(DARKCOIN)
            data[0] = 48 + 28;
        #endif

        #if defined(LITECOIN)
            data[0] = 48;
        #endif

        #if defined(BITCOIN)
            data[0] = 0;
        #endif
        
        #if defined(TESTNET)
            data[0] = 0;
        #endif        
        
        #if defined(FEDORACOIN)
            data[0] = 33;
        #endif

        #if defined(PEERCOIN)
            data[0] = 48 + 7;
        #endif

        #if defined(CLAM)
            data[0] = 137;
        #endif

        #if defined(JUMBUCKS)
            data[0] = 43;
        #endif

        #if defined(DOGECOIN)
            data[0] = 30;
        #endif

        #if defined(MYRIADCOIN)
            data[0] = 50;
        #endif
		
        #if defined(UNOBTANIUM)
            data[0] = 130;
        #endif

        uint8_t sha[kSHA256ByteSize];
        sha256Twice(sha, data, 1+kRIPEMD160ByteSize);

        hashOK =
            sha[0]==checkSumStart[0]  &&
            sha[1]==checkSumStart[1]  &&
            sha[2]==checkSumStart[2]  &&
            sha[3]==checkSumStart[3];

        if(!hashOK) {
            warning(
                "checksum of address %s failed. Expected 0x%x%x%x%x, got 0x%x%x%x%x.",
                addr,
                checkSumStart[0], checkSumStart[1], checkSumStart[2], checkSumStart[3],
                sha[0],           sha[1],           sha[2],           sha[3]
            );
        }
    }

    return hashOK;
}

void hash160ToAddr(
          uint8_t *addr,    // 32 bytes is safe
    const uint8_t *hash160,
          uint8_t type
)
{
    uint8_t buf[4 + 2 + kRIPEMD160ByteSize + kSHA256ByteSize];
    const uint32_t size = 4 + 2 + kRIPEMD160ByteSize;
    buf[ 0] = (size>>24) & 0xff;
    buf[ 1] = (size>>16) & 0xff;
    buf[ 2] = (size>> 8) & 0xff;
    buf[ 3] = (size>> 0) & 0xff;
    buf[ 4] = 0;
    buf[ 5] = type;
    memcpy(4 + 2 + buf, hash160, kRIPEMD160ByteSize);
    sha256Twice(
        4 + 2 + kRIPEMD160ByteSize + buf,
        4 + 1 + buf,
        1 + kRIPEMD160ByteSize
    );

    static BIGNUM *b58 = 0;
    static BIGNUM *num = 0;
    static BIGNUM *div = 0;
    static BIGNUM *rem = 0;
    static BN_CTX *ctx = 0;

    if(!ctx)
    {
        ctx = BN_CTX_new();
        BN_CTX_init(ctx);

        b58 = BN_new();
        num = BN_new();
        div = BN_new();
        rem = BN_new();
        BN_set_word(b58, 58);
    }

    BN_mpi2bn(buf, 4+size, num);

    uint8_t *p = addr;
    while(!BN_is_zero(num))
    {
        int r = BN_div(div, rem, num, b58, ctx);
        if(!r) errFatal("BN_div failed");
        BN_copy(num, div);

        uint32_t digit = BN_get_word(rem);
        *(p++) = b58Digits[digit];
    }

    const uint8_t *a =                          (5+buf);
    const uint8_t *e = 1 + kRIPEMD160ByteSize + (5+buf);
    while(a<e && 0==a[0])
    {
        *(p++) = b58Digits[0];
        ++a;
    }
    *(p--) = 0;

    while(addr<p)
    {
        uint8_t a = *addr;
        uint8_t b = *p;
        *(addr++) = b;
        *(p--) = a;
    }
}

bool guessHash160(
          uint8_t *hash160,
    const uint8_t *addr,
             bool verbose
)
{
    const uint8_t *p = addr;
    while(1) {
        uint8_t c = *p;
        uint8_t h = fromHexDigit(c, false);
        if(0xff==h) break;
        ++p;
    }

    ptrdiff_t size = p - addr;
    if(2*kRIPEMD160ByteSize==size) {
        fromHex(hash160, addr, kRIPEMD160ByteSize, false);
        return true;
    }

    return addrToHash160(hash160, addr, true, verbose);
}

static bool addAddr(
    std::vector<uint160_t> &result,
    const uint8_t *buf,
    bool verbose
)
{
    uint160_t h160;
    bool ok = guessHash160(h160.v, buf, verbose);
    if(ok) result.push_back(h160);
    return ok;
}

void loadKeyList(
    std::vector<uint160_t> &result,
    const char *str,
    bool verbose
)
{
    bool isFile = (
        'f'==str[0] &&
        'i'==str[1] &&
        'l'==str[2] &&
        'e'==str[3] &&
        ':'==str[4]
    );
    if(!isFile) {
        addAddr(result, (uint8_t*)str, true);
        return;
    }

    const char *fileName = 5+str;
    bool isStdIn = ('-'==fileName[0] && 0==fileName[1]);
    FILE *f = isStdIn ? stdin : fopen(fileName, "r");
    if(!f) {
        warning("couldn't open %s for reading\n", fileName);
        return;
    }

    size_t found = 0;
    size_t lineCount = 0;
    double start = usecs();
    while(1) {

        char buf[1024];
        char *r = fgets(buf, sizeof(buf), f);
        if(r==0) break;
        ++lineCount;

        size_t sz = strlen(buf);
        if('\n'==buf[sz-1]) buf[sz-1] = 0;

        uint160_t h160;
        bool ok = addAddr(result, (uint8_t*)buf, verbose);
        if(ok) {
            ++found;
        } else {
            if(verbose) {
                warning(
                    "in file %s, line %d, %s is not an address\n",
                    fileName,
                    lineCount,
                    buf
                );
            }
        }
    }
    fclose(f);

    double elapsed = (usecs() - start)*1e-6;
    info(
        "file %s loaded in %.2f secs, found %d addresses",
        fileName,
        elapsed,
        (int)found
    );
}

void loadHash256List(
    std::vector<uint256_t> &result,
    const char *str,
    bool verbose
)
{
    bool isFile = (
        'f'==str[0] &&
        'i'==str[1] &&
        'l'==str[2] &&
        'e'==str[3] &&
        ':'==str[4]
    );

    if(!isFile) {

        size_t sz = strlen(str);
        if(2*kSHA256ByteSize!=sz) errFatal("%s is not a valid TX hash", str);

        uint256_t h256;
        fromHex(h256.v, (const uint8_t *)str);
        result.push_back(h256);
        return;
    }

    const char *fileName = 5+str;
    bool isStdIn = ('-'==fileName[0] && 0==fileName[1]);
    FILE *f = isStdIn ? stdin : fopen(fileName, "r");
    if(!f) {
        warning("couldn't open %s for reading\n", fileName);
        return;
    }

    size_t lineCount = 0;
    while(1) {

        char buf[1024];
        char *r = fgets(buf, sizeof(buf), f);
        if(r==0) break;
        ++lineCount;

        size_t sz = strlen(buf);
        if(2*kSHA256ByteSize<=sz) {

            uint256_t h256;
            bool ok = fromHex(h256.v, (const uint8_t *)buf, kSHA256ByteSize, true, false);
            if(ok)
                result.push_back(h256);
            else if(verbose) {
                warning(
                    "in file %s, line %d, %s is not a valid TX hash\n",
                    fileName,
                    lineCount,
                    buf
                );
            }
        }

    }
    fclose(f);
}

std::string pr128(
    const uint128_t &y
)
{
    static char result[1024];
    char *p = 1023+result;
    *(p--) = 0;

    uint128_t x = y;
    while(1) {
        *(p--) = (char)((x % 10) + '0');
        if(unlikely(0==x)) break;
        x /= 10;
    }
    ++p;

    return std::string(p[0]!='0' ? p : (1022+result==p) ? p : p+1);
}

void showFullAddr(
    const Hash160 &addr,
    bool both
)
{
    uint8_t b58[128];
    if(both) showHex(addr, sizeof(uint160_t), false);
    hash160ToAddr(b58, addr);
    printf(
        "%s%s",
        both ? " " : "", b58
    );
}

uint64_t getBaseReward(
    uint64_t h
)
{
    static const uint64_t kCoin = 100000000;
    uint64_t reward = (50 * kCoin);
    uint64_t shift = (h/210000);
    reward >>= shift;
    return reward;
}

const char *getInterestingAddr() {

    const char *addr =

    #if defined(BITCOIN)

        "1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp"
        
    #elif defined(TESTNET)
    
        "mxRN6AQJaDi5R6KmvMaEmZGe3n5ScV9u33"

    #elif defined(LITECOIN)

        "LKvTVnkK2rAkJXfgPdkaDRgvEGvazxWS9o"

    #elif defined(DARKCOIN)

        "XnuCAYmAiVHE6Xv3D7Xw685wWzqtcfexLh"

    #elif defined(PEERCOIN)

        "PDH9AeruMUGh2JzYYTpaNtjLAcfGV5LEto"

    #elif defined(CLAM)

        "xQKq1LwJQQkg1A5cmB9znGozCKLkAaKJHW"

    #elif defined(JUMBUCKS)

        "JhbrvAmM7kNpwA6wD5KoAsbtikLWWMNPcM"

    #elif defined(MYRIADCOIN)

        "MDiceoNDTQboRxYKMTstxxRBY493Lg9bV2"

    #elif defined(UNOBTANIUM)

        "udicetdXSo6Zc7vhWgAZfz4XrwagAX34RK"

    #else

        fatal("no address specified")

    #endif
    ;

    warning("no addresses specified, using popular address %s", addr);
    return addr;
}

#if defined(DARKCOIN)

    #include <h9/h9.h>

    void h9(
              uint8_t *h9r,
        const uint8_t *buf,
        uint64_t      size
    ) {
        uint256 hash = Hash9(buf, size + buf);
        memcpy(h9r, &hash, sizeof(hash));
    }

#endif

#if defined(CLAM) || defined(JUMBUCKS)

    #include <scrypt/scrypt.h>

    void scrypt(
              uint8_t *scr,
        const uint8_t *buf,
        uint64_t      size
    ) {
        unsigned char scratchpad[SCRYPT_BUFFER_SIZE];
        uint256 hash = scrypt_nosalt(buf, size, scratchpad);
        memcpy(scr, &hash, sizeof(hash));
    }

#endif

void canonicalHexDump(
    const uint8_t *p,
           size_t size,
       const char *indent
) {
    const uint8_t *s =        p;
    const uint8_t *e = size + p;
    while(p<e) {

        printf(
            "%s%06x: ",
            indent,
            (int)(p-s)
        );

        const uint8_t *lp = p;
        const uint8_t *np = 16 + p;
        const uint8_t *le = std::min(e, 16+p);
        while(lp<np) {
            if(lp<le) printf("%02x ", (int)*lp);
            else      printf("   ");
            ++lp;
        }
        printf("  ");

        lp = p;
        while(lp<le) {
            int c = *(lp++);
            printf("%c", isprint(c) ? c : '.');
        }

        printf("\n");
        p = np;
    }
}

void showScriptInfo(
    const uint8_t *outputScript,
    uint64_t      outputScriptSize,
    const uint8_t *indent
) {

    uint8_t type[128];
    const char *typeName = "unknown";
    uint8_t pubKeyHash[kSHA256ByteSize];
    int r = solveOutputScript(pubKeyHash, outputScript, outputScriptSize, type);

    switch(r) {
        case 0: {
            typeName = "pays to hash160(pubKey)";
            break;
        }
        case 1: {
            typeName = "pays to explicit uncompressed pubKey";
            break;
        }
        case 2: {
            typeName = "pays to explicit compressed pubKey";
            break;
        }
        case 3: {
            typeName = "pays to hash160(script)";
            break;
        }
        case 4: {
            typeName = "pays to hash160(script)";
            break;
        }
        case -2: {
            typeName = "broken script generated by p2pool - coins lost";
            break;
        }
        case -1: {
            typeName = "couldn't parse script";
            break;
        }
    }
    printf(
        "%sscriptType = '%s'\n",
        indent,
        typeName
    );

    if(0<=r) {
        uint8_t btcAddr[64];
        hash160ToAddr(btcAddr, pubKeyHash);
        printf(
            "%sscriptPaysToAddr = '%s'\n",
            indent,
            btcAddr
        );
        printf(
            "%sscriptPaysToHash160 = '",
            indent
        );
        showHex(pubKeyHash, kRIPEMD160ByteSize, false);
        printf("'\n");
    }
}

static inline void writeEscapedChar(
    int  c,
    FILE *f
) {
         if(unlikely(0==c))  { fputc('\\', f); c = '0'; }
    else if(unlikely('\n'==c)) fputc('\\', f);
    else if(unlikely('\t'==c)) fputc('\\', f);
    else if(unlikely('\\'==c)) fputc('\\', f);
    fputc(c, f);
}

void writeEscapedBinaryBufferRev(
    FILE          *f,
    const uint8_t *p,
    size_t        n
)
{
    p += n;

    while(n--) {
        uint8_t c = *(--p);
        writeEscapedChar(c, f);
    }
}

void writeEscapedBinaryBuffer(
    FILE          *f,
    const uint8_t *p,
    size_t        n
)
{
    while(n--) {
        uint8_t c = *(p++);
        writeEscapedChar(c, f);
    }
}

