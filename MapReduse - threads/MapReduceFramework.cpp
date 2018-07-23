#include <pthread.h>
#include <vector>
#include <mutex>
#include <atomic>
#include "Barrier.h"
#include "MapReduceFramework.h"
#include <semaphore.h>
#include <algorithm>
#include <iostream>

#define FIRST_THREAD 0
#define ERROR -1
#define MUTEX_LOCK_ERR "system error: failed to lock mutex"
#define MUTEX_UNLOCK_ERR "system error: failed to unlock mutex"

//......................................... error messages .......................//
const String FAILED_TO_FREE_SEMAPHORE = "system error: failed to free semaphore";
const String FAILED_TO_LOCK_SEMAPHORE = "system error: failed to lock semaphore";
const String MULTILEVEL_THREAD_NUMBER_ERROR = "System error: multiThreadLevel has to be a positive number";
const String THREAD_CREATION_FAILED = "System error: failed to create a new thread";

using namespace std;

/**
 * This structure is responsible to manages each and every
 * one of the thread's context & data.
 */
struct ThreadContext {
    int tid;
    int maxThreads;
    std::atomic<int> *atomic_counter;
    Barrier *barrier;
    sem_t* semaphore;
    pthread_mutex_t *mutex1;

    bool *shuffleStatus; // True when shuffle Phase is still on
    const MapReduceClient *client;
    const InputVec *input;
    std::vector<InputPair> *threadInput;
    std::vector<IntermediateVec *> *interElements; // Vector of intermediate vectors to hold each thread vector
    std::vector<IntermediateVec *> *shuffled; // Shuffled interElement vector
    OutputVec *output;
};

/**
 * reports for an error, and exit the program.
 * @param massege
 */
void error(String massege){
    cerr << massege << endl;
    exit(ERROR);
}

/**
 *  This function compares between the elements pair1, pair2.
 * @param pair1
 * @param pair2
 * @return True if pair2 is bigger, false otherwise.
 */
bool comperator(const pair<const K2 *, const V2 *> &pair1, const pair<const K2 *, const V2 *> &pair2) {
    return *pair1.first < *pair2.first;
}


/**
 *  This function is in charge of processing the input elements by the threads.
 *  Responsible of splitting the input to different threads
 * */
void getInput(ThreadContext *tc) {
    auto inputSize = (*tc->input).size(); // Number of elements to process;
    int oldValue;
    while (true) {
        oldValue = (*tc->atomic_counter)++; // Make sure only one thread gets that index.
        if (oldValue >= (int)inputSize)
            return;
        tc->threadInput->push_back((*tc->input)[oldValue]);
        // Add the input element picked by this thread to the thread's input vector.
    }
}

/**
 * Creates single vector to insert into the shuffle vector.
 * Go over the inter elements
 * @param tc
 * @param key
 * @return
 */
IntermediateVec* createShuffledVec(ThreadContext *tc,K2 *key){
    // Creating the shuffled elements
    auto *shuffledVec = new IntermediateVec;
    auto elements = *tc->interElements;

    for (int thread = 0; thread < tc->maxThreads; thread++) {
        // Arranges the biggest key in sequence from all inter vectors.
        auto threadElements = elements[thread];
        bool isEmpty = (*(threadElements)).empty();
        bool equalBiggest = false;

        if (!isEmpty && &((tc->interElements)[thread]).back() != nullptr)
            equalBiggest = !(*(threadElements->back()).first < *key) &&
                           !(*key < *(threadElements->back()).first);

        while (equalBiggest) {
            (*shuffledVec).emplace_back((*(threadElements)).back());
            (*threadElements).pop_back();

            if ((*threadElements).empty())
                break;

            equalBiggest = !(*(threadElements->back()).first < *key) &&
                           !(*key < *(threadElements->back()).first);
        }
    }
    return shuffledVec;
}

/**
 * Finds the biggest key
 * @return
 */
K2 *findBiggest(){
    K2 *biggestKey = nullptr;
    for (int thread = 0; thread < tc->maxThreads; thread++) {
        auto interVec = (*tc->interElements)[thread];
        if (!(interVec->empty())) {
            auto interElement = (interVec)->back();
            if (biggestKey == nullptr)
                biggestKey = interElement.first;
            else {
                K2 *temp = interElement.first;
                if (*biggestKey < *temp)
                    biggestKey = temp;
            }
        }
    }
    return biggestKey;
}

/**
 * Check weather we are done with
 * the keys for the shuffle phase
 * @return
 */
bool noMoreKeys(){
    return (biggestKey == nullptr);
}


void shuffleDone(ThreadContext *tc){
    for (int i = 0; i <10 ; ++i) {
        sem_post(tc->semaphore);
    }
    if(pthread_mutex_lock(tc->mutex1) != 0){
        cerr << MUTEX_LOCK_ERR << endl;
        exit(ERROR);
    }
    *tc->shuffleStatus = true;
    if(pthread_mutex_unlock(tc->mutex1) != 0) {
        cerr << MUTEX_UNLOCK_ERR << endl;
        exit(ERROR);
    }
}

/**
 * This function is in-charge of the shuffle phase,
 * only one thread executes it
 * @param tc
 */
void shufflePhase(ThreadContext *tc) {

    while (true) {
        K2 *biggestKey = findBiggest();

        if (noMoreKeys(biggestKey)) {// No more keys to shuffle.
            break;
        }

        auto shuffledVec = createShuffledVec(tc, biggestKey);

        if(pthread_mutex_lock(tc->mutex1) != 0){
            cerr << MUTEX_LOCK_ERR << endl;
            exit(ERROR);
        }
        tc->shuffled->push_back(shuffledVec);
        if(sem_post(tc->semaphore) != 0){
            error(FAILED_TO_FREE_SEMAPHORE);
        }
        if(pthread_mutex_unlock(tc->mutex1) != 0) {
            cerr << MUTEX_UNLOCK_ERR << endl;
            exit(ERROR);
        }

    }

    // Sets the shuffle phase as done
    // Releases the threads from the semaphore accordingly
    shufflePhaseDone(tc);
}

/**
 * Checks weather the shuffle phase is still in progress,
 * (we hav'nt doen with adding new vectors to the shuffle vector).
 * @param tc
 * @return true if in progress, false otherwise.
 */
bool shufflePhaseInProgress(ThreadContext* tc){

    if(pthread_mutex_lock(tc->mutex1) != 0){
        cerr << MUTEX_LOCK_ERR << endl;
        exit(ERROR);
    }
    auto check = (!tc->shuffleStatus || !(*tc).shuffled->empty());
    if(pthread_mutex_unlock(tc->mutex1) != 0) {
        cerr << MUTEX_UNLOCK_ERR << endl;
        exit(ERROR);
    }
    return check;
}

/**
 * Free the thread's interElements & shuffle vectors.
 * @param tc
 */
void cleanVecs(ThreadContext *tc){

    tc->interElements->clear();
    tc->shuffled->clear();
    delete tc->interElements;
    delete tc->shuffled;

}

/**
 * Taking care of the reduce phase.
 * @param tc
 */
void reducePhase(ThreadContext *tc){

    while(shufflePhaseInProgress(tc)) {
        if (tc->tid != FIRST_THREAD)
            if (sem_wait(tc->semaphore) != 0) { // Wait if there is no vector to reduce in shuffled
                error(FAILED_TO_LOCK_SEMAPHORE);
            }

        if (pthread_mutex_lock(tc->mutex1) != 0) {
            cerr << MUTEX_LOCK_ERR << endl;
            exit(ERROR);
        }


        if (tc->shuffled->empty()) {
            if(pthread_mutex_unlock(tc->mutex1) != 0)
            {
                cerr << MUTEX_UNLOCK_ERR << endl;
                exit(ERROR);
            }
            continue;
        }
        IntermediateVec *vector = tc->shuffled->back();
        tc->shuffled->pop_back();
        if(pthread_mutex_unlock(tc->mutex1) != 0){
            cerr << MUTEX_UNLOCK_ERR << endl;
            exit(ERROR);
        }

        // Reduce the next shuffled vector
        (tc->client)->reduce(vector, tc);
        delete vector;
    }
}

/**
 * This is the starting point of each one of the pool's threads.
 * @param arg
 * @return
 */
void *startingPoint(void *arg) {

    static atomic<int> check(0);
    auto tc = (ThreadContext *) arg;

    // process input phase
    getInput(tc);

    for (auto element : *(tc->threadInput)) { // The map phase
        (tc->client)->map(element.first, element.second, tc);
    }

    // sort
    auto sortedElements = (*tc->interElements)[tc->tid];
    sort(sortedElements->begin(), sortedElements->end(), comperator);

    // Waits in barrier
    tc->barrier->barrier();

    // Shuffle phase
    if((check++)==0) // here we make sure only one thread is shuffling
        shufflePhase(tc);

    // Reduce phase
    reducePhase(tc);

    delete tc->threadInput;
    return nullptr;
}

void emit2(K2 *key, V2 *value, void *context) {

    ThreadContext *tc = (ThreadContext *) context;
    int tid = tc->tid;
    ((*(tc->interElements))[tid])->push_back(IntermediatePair(key, value));

}

void emit3(K3 *key, V3 *value, void *context) {

    ThreadContext *tc = (ThreadContext *) context;
    if(pthread_mutex_lock(tc->mutex1) != 0){
        cerr << MUTEX_LOCK_ERR << endl;
        exit(ERROR);
    }
    tc->output->push_back(pair<K3 *, V3 *>(key, value));
    if(pthread_mutex_unlock(tc->mutex1) != 0){
        cerr << MUTEX_UNLOCK_ERR << endl;
        exit(ERROR);
    }
}

/**
 * This is the main function. it creates the threads
 * using the relevant input for each one of them.
 * @param client the specific client class.
 * @param inputVec the input vector
 * @param outputVec
 * @param multiThreadLevel number of threads in the pool
 */
void runMapReduceFramework(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec,
                           int multiThreadLevel) {

    if(multiThreadLevel <= 0){
        error(MULTILEVEL_THREAD_NUMBER_ERROR);
    }

    int maxThreads = multiThreadLevel;
    pthread_t threads[maxThreads]; // Create a thread array of maxThreads length.
    ThreadContext contexts[maxThreads]; // Create context for each thread.

    // Initialize multi-threading mechanisms.
    Barrier barrier(maxThreads);
    atomic<int> atomic_counter(0);
    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

    bool shuffleVar = false;
    sem_t semaphore{};
    sem_init(&semaphore, 0, 0);

    auto inter = new std::vector<IntermediateVec *>();
    for (int i = 0; i < maxThreads; i++) // Set a vector of intermediate vectors for each thread.
    {
        inter->push_back(new IntermediateVec);
    }
    std::vector<IntermediateVec*> *shuffled = new std::vector<IntermediateVec *>();


    // Set the Thread Context for each thread
    for (int i = 0; i < maxThreads; ++i) {
        contexts[i] = {i,
                       maxThreads,
                       &atomic_counter,
                       &barrier,
                       &semaphore,
                       &mutex1,
                       &shuffleVar,
                       &client,
                       &inputVec,
                       new std::vector<InputPair>, // each vector gets its own empty inputVec
                       inter,
                       shuffled,
                       &outputVec};
    }

    // Create threads
    for (int i = 1; i < multiThreadLevel; ++i) {
        if(pthread_create(threads + i, nullptr, startingPoint, contexts + i) != 0)
        {
            error(THREAD_CREATION_FAILED);
        }

    }

    startingPoint((void *) &contexts[FIRST_THREAD]);

    for (int i = 1; i < multiThreadLevel; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    cleanVecs(&contexts[0]);
    pthread_mutex_destroy(&mutex1);
    sem_destroy(&semaphore);
}


