#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

using std::cout;
using std::endl;
using std::find_if;
using std::make_unique;
using std::pow;
using std::sqrt;
using std::unique_ptr;
using std::vector;

typedef uint32_t uint;
typedef vector<float> Population;
typedef std::pair<float, float> ChromoPair;

const float CROSSING_CHANCE = 0.9;
const float MUTATION_CHANCE = 0.05;

const uint BYTE_SIZE = 8; // in bits
const uint FLOAT_SIZE = sizeof(float) * BYTE_SIZE;
const uint UINT_SIZE = sizeof(uint) * BYTE_SIZE;
const uint POPULATION_SIZE = 20;

// Randomization
std::random_device rd;
std::mt19937 mt(rd());

//!
//! \param y
//! \return y^7-y^5+5*sqrt(y)
//!
float f(float y)
{
    return pow(y, 7) + pow(y, 5) + 5 * sqrt(y);
}

uint randomUIntDist(uint from, uint to)
{
    std::uniform_int_distribution dist(from, to);
    return dist(mt);
}

float randomFloatDist(float from, float to)
{
    std::uniform_real_distribution<float> dist(from, to);
    return dist(mt);
}

unique_ptr<Population> getFirstPopulation(uint populationSize = 1)
{
    unique_ptr<Population> p = make_unique<Population>();
    for (uint i = 0; i < populationSize; ++i)
        p->push_back(randomFloatDist(5.0, 20.0));

    return p;
}

uint generateCrossMask(uint breakPoint = 0)
{
    uint crossMask {};
    for (uint i = 0; i < breakPoint; ++i)
        crossMask ^= static_cast<uint>(1) << (UINT_SIZE - i - 1);

    return crossMask;
}

ChromoPair cross(ChromoPair pair = { 0.0, 0.0 })
{
    uint parentA = reinterpret_cast<uint&>(pair.first),
         parentB = reinterpret_cast<uint&>(pair.second);

    uint breakPoint = randomUIntDist(0, FLOAT_SIZE - 1);
    uint crossMask = generateCrossMask(breakPoint);

    uint childA = (parentA & crossMask) | (parentB & (~crossMask)),
         childB = (parentB & crossMask) | (parentA & (~crossMask));

    pair.first = reinterpret_cast<float&>(childA);
    pair.second = reinterpret_cast<float&>(childB);

    return pair;
}

float mutate(float parentChromosome = 0.0)
{
    uint randomBit = randomUIntDist(0, FLOAT_SIZE - 1);
    uint parent = reinterpret_cast<uint&>(parentChromosome);
    uint child = parent ^ (1 << (UINT_SIZE - randomBit - 1));
    return reinterpret_cast<float&>(child);
}

//!
//! \brief nextPopulation uses roulette wheel selection principle
//! \param prevPopulation
//! \return next generation
//!
unique_ptr<Population> nextPopulation(unique_ptr<Population>& prevPopulation)
{
    unique_ptr<Population> nextP = make_unique<Population>();

    // #1 Getting the distribution. Chromosomes selection probability
    vector<float> distribution {};
    float sum = std::accumulate(prevPopulation->begin(), prevPopulation->end(), 0);
    for (double ch : *prevPopulation)
        distribution.push_back(ch / sum);

    // #2 Round the wheel n times
    vector<float> parrentPull {};
    for (uint i = 0; i < prevPopulation->size(); ++i) {
        float spin = randomFloatDist(0.0, 1.0);
        uint winner {};
        do {
            if (winner == distribution.size())
                break;

            spin -= distribution.at(winner);
            winner++;
        } while (spin > 0);

        parrentPull.push_back(prevPopulation->at(winner - 1));
    }

    // #3 Crossings and mutations
    for (double ch : parrentPull) {

        // Creating pair
        uint randomParent = randomUIntDist(0, prevPopulation->size() - 1);
        ChromoPair pair { ch, prevPopulation->at(randomParent) };

        // Crossing
        {
            // Probability to cross
            float prob = randomFloatDist(0.0, 1.0);

            if (prob < CROSSING_CHANCE)
                pair = cross(pair);
        }

        // Mutating
        {
            // Probability to mutate
            float probA = randomFloatDist(0.0, 1.0);
            float probB = randomFloatDist(0.0, 1.0);

            if (probA < MUTATION_CHANCE)
                pair.first = mutate(pair.first);

            if (probB < MUTATION_CHANCE)
                pair.second = mutate(pair.second);
        }

        // Getting Chromosome
        {
            // Throwing coin
            float prob = randomFloatDist(0.0, 1.0);

            if (prob < 0.5)
                nextP->push_back(pair.first);
            else
                nextP->push_back(pair.second);
        }
    }
    return nextP;
}

int main()
{
    const float maximum = f(20.0);
    const uint n = 1;
    const float precision = 1 / pow(10, n);

    // #1 Creating first population
    unique_ptr<Population> p = getFirstPopulation(POPULATION_SIZE);

    // #2 Generating populations
    auto isEnoughPrecise = [&](float ch) {
        bool result = (ch >= maximum - precision) && (ch <= maximum + precision);
        return result;
    };

    uint index {};
    do {
        index++;
        p = nextPopulation(p);

        cout << endl
             << "------------------------------------------" << endl;
        cout << "Iteration #" << index << endl;
        cout << "Fitness: " << index << endl;
        cout << "Results: ";

        for (double ch : *p)
            cout << ch << " ";

    } while (find_if(p->begin(), p->end(), isEnoughPrecise) == p->end());

    cout << endl
         << endl;
    cout << "Cool! We found enough precise value in " << index << " generation!" << endl;
    cout << "Real anwer is: " << maximum << endl;

    return 0;
}
