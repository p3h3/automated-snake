#include <stdlib.h>

// Modified code from http://www.martinbroadhurst.com/hamiltonian-circuits-using-backtracking-in-c.html
static unsigned int circuit_contains(const unsigned int *circuit, unsigned int c, unsigned int v){
    unsigned int contains = 0;
    for (unsigned int i = 0; i < c && !contains; i++) {
        contains = circuit[i] == v;
    }
    return contains;
}

//Gets called every time a valid circuit is found
typedef void (*circuitfn)(const unsigned int *, size_t);
 
static void recursive_hamiltonian_circuits(unsigned int **adjmat, size_t n, unsigned int *circuit,
       unsigned int count, circuitfn fun) {
    if (count == n) {
        /* Found a circuit */
        fun(circuit, n);
    }
    else {
        for (unsigned int v = 1; v < n; v++) {
            if (!circuit_contains(circuit, count, v)      // Vertex is not in the circuit already
                && adjmat[circuit[ count - 1]][v] == 1    // Vertex is adjacent (connected) to the previous vertex
                && (count < n - 1 || (adjmat[0][v] == 1   // Not the last Vertex OR last vertex is adjacent (connected) to the first
                    && v < circuit[1])))                  // Last vertex is less than the second
            {
                circuit[count] = v;
                recursive_hamiltonian_circuits(adjmat, n, circuit, count + 1, fun);
            }
        }
    }
} 
 
void hamiltonian_circuits(unsigned int **adjmat, size_t n, circuitfn fun) {
    unsigned int *circuit;
    circuit = malloc(n * sizeof(unsigned int));
    if (circuit == NULL) {
        return;
    }
    circuit[0] = 0;
    recursive_hamiltonian_circuits(adjmat, n, circuit, 1, fun);
    free(circuit);
}
