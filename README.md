# CPU Scheduler Simulator: preemptive scheduler con Shortest Job First e quantum prediction
## Liwen Zheng

L'algoritmo **`Shortest Job First`** per la scelta del processo da eseguire consiste nello scegliere il processo con il `minor CPU burst`, cioè quello che terrà meno occupata la CPU.
Il limite di questo algoritmo sta nell'_impossibilità di conoscere il futuro comportamento di un processo_ e quindi di stabilirne i tempi di CPU e IO burst.

E' possibile rimediare a questo problema cercando di prevedere il futuro comportamento del processo che, tendenzialmente, è **ciclico**: le durate dei CPU burst potranno variare ma si terranno mediamente su un certo valore così come per gli IO burst.

Una possibile approssimazione si ottiene applicando un _filtro passa basso_ in grado di attenuare l'effetto di burst eccezionali. Il tempo stimato del prossimo burst sarà dunque una media tra il tempo effettivamente misurato del burst corrente ed il tempo precedentemente stimato per il burst corrente:
```
B(t+1) = α * b(t) + (1-α) * B(t)
```
Il ***coefficiente di decay*** *`α`* serve ad attribuire nel calcolo della media un peso al ***tempo misurato*** *`b(t)`* e al ***tempo precedentemente stimato*** *`B(t)`*.

Nell'algoritmo che andremo ad implementare, applicheremo all'idea dello Shortest Job First il concetto di **`preemption`**: lo scheduler può togliere forzatamente la CPU ad un processo se questo la sta usando da più di un periodo di tempo chiamato **`cpu quantum`**.

Per integrare il concetto di preemption con lo SJB, andremo a predire il prossimo CPU burst attraverso il filtro passa basso, e considereremo questo valore come il quanto di tempo dopo il quale verrà tolta la cpu al processo corrente.
Nel caso base, cioè quando un processo arriva, useremo la durata reale dell'evento come quantum prediction.
