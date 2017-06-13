#define SEND TP.send("Hello!");
#define RECEIVE [] TP.receive(?) -> value vl_string \
        { Tl_t.stop; log(vl_string); setverdict(pass); }
#define TIMEOUT [] Tl_t.timeout { setverdict(fail); }

