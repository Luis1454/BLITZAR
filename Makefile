# Nom de l'exécutable à générer
EXECUTABLE := myApp

# Compilateurs
NVCC := nvcc
CXX := /usr/bin/g++-13.2
CC := g++

# Options de compilation
NVCC_FLAGS := -ccbin $(CXX)
LDFLAGS := -L/usr/local/cuda/lib64 -lcudart -lsfml-graphics -lsfml-window -lsfml-system

# Fichiers source
CUDA_SRC := test.cu
CXX_SRC := main.cpp Particle.cpp ParticleSystem.cpp

# Fichiers objet
CUDA_OBJ := $(CUDA_SRC:.cu=.o)
CXX_OBJ := $(CXX_SRC:.cpp=.o)

# Règle par défaut
all: $(EXECUTABLE)

# Compilation CUDA
%.o: %.cu
	$(NVCC) $(NVCC_FLAGS) -c $< -o $@

# Compilation C++
%.o: %.cpp
	$(CC) -c $< -o $@

# Édition de liens
$(EXECUTABLE): $(CUDA_OBJ) $(CXX_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

# Nettoyage
clean:
	rm -f $(EXECUTABLE) $(CUDA_OBJ) $(CXX_OBJ)

fclean: clean
	rm -f $(CUDA_OBJ) $(CXX_OBJ)

re: clean all

.PHONY: all clean