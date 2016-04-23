#include <cstdlib>
#include <iostream>
#include "lwe.h"
#include "lweparams.h"
#include "lwekey.h"
#include "lwesamples.h"
#include "ringlwe.h"
#include <random>

using namespace std;

static default_random_engine generator;
static const int64_t _two31 = INT64_C(1) << 31; // 2^31
static const int64_t _two32 = INT64_C(1) << 32; // 2^32
static const double _two32_double = _two32;
static const double _two31_double = _two31;


// from double to Torus32
EXPORT Torus32 dtot32(double d) {
    return int32_t(int64_t((d - int64_t(d))*_two32));
}
// from Torus32 to double
EXPORT double t32tod(Torus32 x) {
    return double(x)/_two32_double;
}


// Gaussian sample centered in message, with standard deviation sigma
Torus32 gaussian32(Torus32 message, double sigma){
    //Attention: all the implementation will use the stdev instead of the gaussian fourier param
    normal_distribution<double> distribution(0.,sigma); //TODO: can we create a global distrib of param 1 and multiply by sigma?
    double err = distribution(generator);
    return message + dtot32(err);
}


// Used to approximate the phase to the nearest message possible in the message space
// The constant Msize will indicate on which message space we are working (how many messages possible)
Torus32 approxPhase(Torus32 phase, int Msize){
    uint64_t interv = UINT64_C(-1)/Msize; // width of each intervall
    uint64_t half_interval = interv/2; // begin of the first intervall
    uint64_t phase64 = (uint64_t(phase)<<32) + half_interval;
    //floor to the nearest multiples of interv
    phase64 -= phase64%interv;
    //rescale to torus32
    return int32_t(phase64>>32); 
}







/**
 * This function generates a random LWE key for the given parameters.
 * The LWE key for the result must be allocated and initialized
 * (this means that the parameters are already in the result)
 */
EXPORT void lweKeyGen(LWEKey* result) {
  int n = result->params->n;
  uniform_int_distribution<int> distribution(0,1);

  for (int i=0; i<n; i++) 
    result->key[i]=distribution(generator);
}



/**
 * This function encrypts message by using key, with stdev alpha
 * The LWE sample for the result must be allocated and initialized
 * (this means that the parameters are already in the result)
 */
EXPORT void lweSymEncrypt(LWESample* result, Torus32 message, double alpha, const LWEKey* key){
    int n = key->params->n;
    uniform_real_distribution<double> distribution(-0.5,0.5);

    result->b = gaussian32(message, alpha); 
    for (int i = 0; i < n; ++i)
    {
        result->a[i] = dtot32(distribution(generator));
        result->b += result->a[i]*key->key[i];
    }
}



/**
 * This function computes the phase of sample by using key : phi = b - a.s
 */
EXPORT Torus32 lwePhase(const LWESample* sample, const LWEKey* key){
    const int n = key->params->n;
    Torus32 phi = 0;
    const Torus32 *__restrict a = sample->a;
    const int * __restrict k = key->key;

    for (int i = 0; i < n; ++i) 
	   phi += a[i]*k[i]; 
    return sample->b - phi;
}


/**
 * This function computes the decryption of sample by using key
 * The constant Msize indicates the message space and is used to approximate the phase
 */
EXPORT Torus32 lweSymDecrypt(const LWESample* sample, const LWEKey* key, const int Msize){
    Torus32 phi;

    phi = lwePhase(sample, key);
    return approxPhase(phi, Msize);
}








//voir si on le garde ou on fait lweAdd (laisser en suspense)
EXPORT void lweLinearCombination(LWESample* result, const int* combi, const LWESample** samples, const LWEParams* params);

EXPORT void lweKeySwitch(LWESample* result, const LWEKeySwitchKey* ks, const LWESample* sample);







// RingLWE
EXPORT void ringLweKeyGen(RingLWEKey* result){
  int N = result->params->N;
  int k = result->params->k;
  uniform_int_distribution<int> distribution(0,1);

  for (int i = 0; i < k; ++i)
      for (int j = 0; j < N; ++j)
          result->key->coefs[i][j] = distribution(generator);
}




EXPORT void ringLweSymEncrypt(RingLWESample* result, TorusPolynomial* message, const RingLWEKey* key){
    /*
    int N = key->params->N;
    int k = key->k;
    uniform_real_distribution<double> distribution(-0.5,0.5);
    TorusPolynomial* temp;

    for (int j = 0; j < N; ++j)
        result->b[j] = gaussian32(0, alpha) + message[j];   
    
    for (int i = 0; i < k; ++i)
    {
        for (int j = 0; j < N; ++j)
            result->a[i][j] = dtot32(distribution(generator));
        multKaratsuba(temp, key->key[i], result->a[i]);

    }
    */
    
}








EXPORT double ringLweSymDecrypt(const RingLWESample* sample, const RingLWEKey* key);

EXPORT void ringLwePolyCombination(RingLWESample* result, const int* combi, const RingLWESample* samples, const RingLWEParams* params);

// RingGSW
EXPORT void ringGswKeyGen(LWEKey* result);
EXPORT void ringGswSymEncrypt(LWESample* result, double message, const LWEKey* key);
EXPORT double ringGswSymDecrypt(const LWESample* sample, const LWEKey* key);

EXPORT void ringGswPolyCombination(LWESample* result, const int* combi, const LWESample* samples, const LWEParams* params);

//extractions Ring LWE -> LWE
EXPORT void keyExtract(LWEKey* result, const RingLWEKey*); //sans doute un param supplémentaire
EXPORT void sampleExtract(LWESample* result, const RingLWESample* x);

//extraction RingGSW -> GSW
EXPORT void gswKeyExtract(RingLWEKey* result, const RingGSWKey* key); //sans doute un param supplémentaire
EXPORT void gswSampleExtract(RingLWESample* result, const RingGSWSample* x);

//bootstrapping
EXPORT void bootstrap(LWESample* result, const LWEBootstrappingKey* bk, double mu1, double mu0, const LWESample* x);
