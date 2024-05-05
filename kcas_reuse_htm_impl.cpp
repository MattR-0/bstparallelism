#include "kcas_reuse_htm_impl.h"

thread_local TIDGenerator kcas_tid;

bool isRdcss(casword_t val) {
    return (val & RDCSS_TAGBIT);
}

bool isKcas(casword_t val) {
    return (val & KCAS_TAGBIT);
}

template <int MAX_K>
void KCASHTM<MAX_K>::rdcssHelp(rdcsstagptr_t tagptr, rdcssptr_t snapshot, bool helpingOther) {
    bool readSuccess;
    casword_t v = DESC_READ_FIELD(readSuccess, *snapshot->addr1, snapshot->old1, KCAS_SEQBITS_MASK_STATE, KCAS_SEQBITS_OFFSET_STATE);
    if (!readSuccess) v = KCAS_STATE_SUCCEEDED; // return;

    if (v == KCAS_STATE_UNDECIDED) {
        BOOL_CAS(snapshot->addr2, (casword_t) tagptr, snapshot->new2);
    } else {
        // the "fuck it i'm done" action (the same action you'd take if the kcas descriptor hung around indefinitely)
        BOOL_CAS(snapshot->addr2, (casword_t) tagptr, snapshot->old2);
    }
}

template <int MAX_K>
void KCASHTM<MAX_K>::rdcssHelpOther(rdcsstagptr_t tagptr) {
    rdcssdesc_t newSnapshot;
    const int sz = rdcssdesc_t::size;
    if (DESC_SNAPSHOT(rdcssdesc_t, rdcssDescriptors, &newSnapshot, tagptr, sz)) {
        rdcssHelp(tagptr, &newSnapshot, true);
    }
}

template <int MAX_K>
casword_t KCASHTM<MAX_K>::rdcss(rdcssptr_t ptr, rdcsstagptr_t tagptr) {
    casword_t r;
    do {
        r = VAL_CAS(ptr->addr2, ptr->old2, (casword_t) tagptr);
        if (isRdcss(r)) {
            rdcssHelpOther((rdcsstagptr_t) r);
        }
    } while (isRdcss(r));
    if (r == ptr->old2) rdcssHelp(tagptr, ptr, false); // finish our own operation
    return r;
}

template <int MAX_K>
casword_t KCASHTM<MAX_K>::rdcssRead(casword_t volatile * addr) {
    casword_t r;
    do {
        r = *addr;
        if (isRdcss(r)) {
            rdcssHelpOther((rdcsstagptr_t) r);
        }
    } while (isRdcss(r));
    return r;
}

template <int MAX_K>
KCASHTM<MAX_K>::KCASHTM() {
    DESC_INIT_ALL(kcasDescriptors, KCAS_SEQBITS_NEW);
    DESC_INIT_ALL(rdcssDescriptors, RDCSS_SEQBITS_NEW);
}

template <int MAX_K>
void KCASHTM<MAX_K>::helpOther(kcastagptr_t tagptr) {
    kcasdesc_t<MAX_K> newSnapshot;
    const int sz = kcasdesc_t<MAX_K>::size;
    //cout<<"size of kcas descriptor is "<<sizeof(kcasdesc_t<MAX_K>)<<" and sz="<<sz<<endl;
    if (DESC_SNAPSHOT(kcasdesc_t<MAX_K>, kcasDescriptors, &newSnapshot, tagptr, sz)) {
        help(tagptr, &newSnapshot, true);
    }
}

template <int MAX_K>
bool KCASHTM<MAX_K>::help(kcastagptr_t tagptr, kcasptr_t snapshot, bool helpingOther) {
    // phase 1: "locking" addresses for this kcas
    int newstate;

    // read state field
    kcasptr_t ptr = TAGPTR_UNPACK_PTR(kcasDescriptors, tagptr);
    bool successBit;
    int state = DESC_READ_FIELD(successBit, ptr->seqBits, tagptr, KCAS_SEQBITS_MASK_STATE, KCAS_SEQBITS_OFFSET_STATE);
    if (!successBit) {
        assert(helpingOther);
        return false;
    }

    if (state == KCAS_STATE_UNDECIDED) {
        newstate = KCAS_STATE_SUCCEEDED;
        for (int i = helpingOther; i < snapshot->numEntries; i++) {
            retry_entry:
            // prepare rdcss descriptor and run rdcss
            rdcssdesc_t *rdcssptr = DESC_NEW(rdcssDescriptors, RDCSS_SEQBITS_NEW, kcas_tid.getId());
            rdcssptr->addr1 = (casword_t*) &ptr->seqBits;
            rdcssptr->old1 = tagptr; // pass the sequence number (as part of tagptr)
            rdcssptr->old2 = snapshot->entries[i].oldval;
            rdcssptr->addr2 = snapshot->entries[i].addr; // p stopped here (step 2)
            rdcssptr->new2 = (casword_t) tagptr;
            DESC_INITIALIZED(rdcssDescriptors, kcas_tid.getId());

            casword_t val;
            val = rdcss(rdcssptr, TAGPTR_NEW(kcas_tid.getId(), rdcssptr->seqBits, RDCSS_TAGBIT));

            // check for failure of rdcss and handle it
            if (isKcas(val)) {
                // if rdcss failed because of a /different/ kcas, we help it
                if (val != (casword_t) tagptr) {
                    helpOther((kcastagptr_t) val);
                    goto retry_entry;
                }
            } else {
                if (val != snapshot->entries[i].oldval) {
                    newstate = KCAS_STATE_FAILED;
                    break;
                }
            }
        }
        SEQBITS_CAS_FIELD(successBit
        , ptr->seqBits, snapshot->seqBits
        , KCAS_STATE_UNDECIDED, newstate
        , KCAS_SEQBITS_MASK_STATE, KCAS_SEQBITS_OFFSET_STATE);
    }
    // phase 2 (all addresses are now "locked" for this kcas)
    state = DESC_READ_FIELD(successBit, ptr->seqBits, tagptr, KCAS_SEQBITS_MASK_STATE, KCAS_SEQBITS_OFFSET_STATE);
    if (!successBit) return false;

    bool succeeded = (state == KCAS_STATE_SUCCEEDED);

    for (int i = 0; i < snapshot->numEntries; i++) {
        casword_t newval = succeeded ? snapshot->entries[i].newval : snapshot->entries[i].oldval;
        BOOL_CAS(snapshot->entries[i].addr, (casword_t) tagptr, newval);
    }

    return succeeded;
}

// TODO: replace crappy bubblesort with something fast for large MAX_K (maybe even use insertion sort for small MAX_K)
template <int MAX_K>
static void kcasdesc_sort(kcasptr_t ptr) {
    kcasentry_t temp;
    for (int i = 0; i < ptr->numEntries; i++) {
        for (int j = 0; j < ptr->numEntries - i - 1; j++) {
            if (ptr->entries[j].addr > ptr->entries[j + 1].addr) {
                temp = ptr->entries[j];
                ptr->entries[j] = ptr->entries[j + 1];
                ptr->entries[j + 1] = temp;
            }
        }
    }
}

template <int MAX_K>
bool KCASHTM<MAX_K>::execute() {
    assert(kcas_tid.getId() != -1);
    
    auto desc = &kcasDescriptors[kcas_tid.getId()];
    // sort entries in the kcas descriptor to guarantee progress
    
    DESC_INITIALIZED(kcasDescriptors, kcas_tid.getId());
    kcastagptr_t tagptr = TAGPTR_NEW(kcas_tid.getId(), desc->seqBits, KCAS_TAGBIT);
    
    for(int i = 0; i < MAX_RETRIES; i++) {
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
	    for (int j = 0; j < desc->numEntries; j++) {
		if(*desc->entries[j].addr == desc->entries[j].oldval){
		    *desc->entries[j].addr = desc->entries[j].newval;
		}
		else {
		    _xabort(HTM_BAD_OLD_VAL);
		}
	    }
	    _xend();
	    return true;
	}
	else {		    	
	    if(_XABORT_CODE(status) == HTM_BAD_OLD_VAL){
		return false;
	    }
	}
    }
    
    kcasdesc_sort<MAX_K>(desc);
    return help(tagptr, desc, false);
    
}

template <int MAX_K>
casword_t KCASHTM<MAX_K>::readPtr(casword_t volatile * addr) {
    casword_t r;
    do {
        r = rdcssRead(addr);
        if (isKcas(r)) {
	    helpOther((kcastagptr_t) r);	    
        }
    } while (isKcas(r));
    return r;
}

template <int MAX_K>
casword_t KCASHTM<MAX_K>::readVal( casword_t volatile * addr) {
    return ((casword_t) readPtr(addr))>>KCAS_LEFTSHIFT;
}

template <int MAX_K>
void KCASHTM<MAX_K>::writeInitPtr(casword_t volatile * addr, casword_t const newval) {
    *addr = newval;
}

template <int MAX_K>
void KCASHTM<MAX_K>::writeInitVal(casword_t volatile * addr, casword_t const newval) {
    writeInitPtr(addr, newval<<KCAS_LEFTSHIFT);
}

template <int MAX_K>
void KCASHTM<MAX_K>::start() {
    // allocate a new kcas descriptor
    kcasptr_t ptr = DESC_NEW(kcasDescriptors, KCAS_SEQBITS_NEW, kcas_tid.getId());
    ptr->numEntries = 0;  
}

template <int MAX_K>
kcasptr_t KCASHTM<MAX_K>::getDescriptor() {
    return &kcasDescriptors[kcas_tid.getId()];
}


template <int MAX_K>
void KCASHTM<MAX_K>::deinitThread() {
    kcas_tid.explicitRelease();
}

template<int MAX_K>
template<typename T>
void KCASHTM<MAX_K>::add(casword<T> * caswordptr, T oldVal, T newVal) {
    caswordptr->addToDescriptor(oldVal, newVal);
}
template<int MAX_K>
template<typename T, typename... Args>
void KCASHTM<MAX_K>::add(casword<T> * caswordptr, T oldVal, T newVal, Args... args) {
    caswordptr->addToDescriptor(oldVal, newVal);
    add(args...);
}


KCASHTM<100000> kcasInstance;

template <typename T>
casword<T>::casword(){
    T a;
    bits =  bits = CASWORD_CAST(a);
}

template <typename T>
T casword<T>::setInitVal(T other) {
    if(is_pointer<T>::value){
	bits = CASWORD_CAST(other);
    }
    else {
	bits = CASWORD_CAST(other);
	assert((bits & 0xE000000000000000) == 0);
	bits = bits << SHIFT_BITS;
    }

    return other;
}

template <typename T>
casword<T>::operator T() {
    if(is_pointer<T>::value){
	return (T)kcasInstance.readPtr(&bits);
    }
    else {
	return (T)kcasInstance.readVal(&bits);
    }
}

template <typename T>
T casword<T>::operator->() {
    assert(is_pointer<T>::value);
    return *this;
}

template <typename T>
T casword<T>::getValue(){
    if(is_pointer<T>::value){
	return (T)kcasInstance.readPtr(&bits);
    }
    else {
	return (T)kcasInstance.readVal(&bits);
    }
}

template <typename T>
void casword<T>::addToDescriptor(T oldVal, T newVal){
    auto descriptor = kcasInstance.getDescriptor();
    auto c_oldVal = (casword_t)oldVal;
    auto c_newVal = (casword_t)newVal;
    assert(((c_oldVal & 0xE000000000000000) == 0) && ((c_newVal & 0xE000000000000000) == 0));

    if(is_pointer<T>::value){
	descriptor->addPtrAddr(&bits, c_oldVal, c_newVal);
    }
    else {
	descriptor->addValAddr(&bits, c_oldVal, c_newVal);
    }
}