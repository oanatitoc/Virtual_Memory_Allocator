Nume: Titoc Oana Alexandra
Grupa: 313CA

Virtual Memory Allocator - Tema1

Descriere:

Alocatorul virtual de memorie pe care a trebuit sa il implementez a fost
practic, o lista de liste. Am folosit structurile si functiile generice de
la liste dublu inlantuite. Scopul acestei teme a fost sa realizam diferite
operatii pe acele blocuri de memorie, reusind in final sa nu avem memory
leakuri sau segmentation fault :).

Programul meu contine in fisierul main.c un meniu prin care se citesc diferite
comenzi pana la intalnirea comenzii "DEALLOC_ARENA" care elibereaza toata
memoria programului si inchide executia acestuia. In "vma.c" se gaseste
implementarea tuturor functiilor a caror apel se face in "vma.h" alaturi de
strucutrile de date folosite.

Prima comanda pe care o primeste programul este "ALLOC_ARENA <dimensiune>".
In implementare nu am folosit efectiv alocarea dinamica a arenei  cu
dimensiunea data cu malloc(), ci am alocat structura arena_t si am initializat
campurile acesteia.

Pentru a doua comanda "ALLOC_BLOCK <adresa> <dimensiune>" am implementat 3
functii suplimentare. De fiecare data am introdus un nou miniblock in arena,
pe care fie l-am lipit la inceputul unui block existent: "add_miniblock_first",
fie l-am adaugat la final: "add_miniblock_last" avand grija ca in caz ca acel
miniblock face legatura intre doua block-uri, ele sa se lipeasca si sa formeze
un singur block. A treia functie, "add_block" creeaza si block-ul izolat pe
care il introduce in lista de block-uri.

Operatia "FREE_BLOCK <adresa>" este fix operatia inversa celei de ALLOC_BLOCK.

Operatiile de READ si WRITE sunt si ele oarecum complementare, la operatia de
WRITE am scris la o adresa date de o anumita dimensiune. Daca acel block era
insuficient ca dimensiune pentru datele care trebuiau scrise, se scriau doar
cat permitea dimensiunea acestuia. Aceste doua functii sunt in stransa legatura
cu functia "MPROTECT" care furnizeaza permisiunile unui anumit miniblock.
Astfel, la apelarea lui read si write se verifica mereu daca miniblock-urile
din care se citeste/scrie au permisiunile necesare de citire/scriere.

Functia PMAP este folosita pentru inspectarea detaliata a arenei: dimensiune,
spatiu liber, numar de block-uri si miniblock-uri, precum si permisiunile
acestora.

Implementarea functiilor FREE_BLOCK si ALLOC_BLOCK putea fi mai eficienta pentru
cazurile in care impartim un block in doua (functia free_from_inside),
respectiv unim doua blocuri in unul singur (functia add_miniblock_last).

Prin aceasta tema am aprofundat cunostiintele legate de structuri de date
generice si de liste reusind sa inteleg mult mai bine importanta acestora
si functionalitatea lor.

