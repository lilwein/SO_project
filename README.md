# CPU Scheduler Simulator
## Preemptive Scheduler con Shortest Job First e Quantum Prediction

L'algoritmo **`Shortest Job First`** per la scelta del processo da eseguire consiste nello scegliere il processo con il `minor CPU burst`, cioè quello che terrà meno occupata la CPU.
Il limite di questo algoritmo sta nell'_impossibilità di conoscere il futuro comportamento di un processo_ e quindi di stabilirne i tempi di CPU e IO burst.

E' possibile rimediare a questo problema cercando di prevedere il futuro comportamento del processo che, tendenzialmente, è **ciclico**: le durate dei CPU burst potranno variare ma si terranno mediamente su un certo valore così come per gli IO burst.

Una possibile approssimazione si ottiene applicando un _filtro passa basso_ in grado di attenuare l'effetto di burst eccezionali. Il tempo stimato del prossimo burst sarà dunque una media tra il tempo effettivamente misurato del burst corrente ed il tempo precedentemente stimato per il burst corrente:
```
B(t+1) = α * b(t) + (1-α) * B(t)
```
Il ***coefficiente di decay*** *`α`* serve ad attribuire nel calcolo della media un peso al ***tempo misurato*** *`b(t)`* e al ***tempo precedentemente stimato*** *`B(t)`*.

Nell'algoritmo implementato, si applica all'idea dello Shortest Job First il concetto di **`preemption`**: lo scheduler può togliere forzatamente la CPU ad un processo se questo la sta usando da più di un periodo di tempo chiamato **`cpu quantum`**.

Per integrare il concetto di preemption con lo SJB, andremo a predire il prossimo CPU burst attraverso il filtro passa basso, e considereremo questo valore come il quanto di tempo dopo il quale verrà tolta la cpu al prossimo processo.

Nel caso di un processo appena creato, poiché il suo primo CPU burst non ha una predizione fatta nel passato, useremo come quanto di tempo la durata dell'intero evento CPU o il valore di un certo valore massimo per il quanto, indicato con *`max_quantum`*, se questo è minore della durata dell'evento.

In ogni caso il valore di *`max_quantum`*, impostato dal kernel a tempo di compilazione, è il tempo massimo dopo il quale un processo in running viene interrotto. Tuttavia il valore dei quanti di tempo calcolati rimangono tali anche se superiori a *max_quantum*, al fine di svolgere correttamente il calcolo dei successivi quanti.

Nel caso in cui un evento di tipo CPU venga interrotto una o più volte perchè in running da un tempo maggoire o uguale al quanto (o *max_quantum*), al momento dell'interruzione (end quantum) l'evento verrà sostituito con un nuovo evento CPU con durata pari alla durata rimanente e quanto pari a
```
lpFilter = pcb->timer_CPU_burst * args->decay + e->quantum * (1-args->decay)
```
In altri termini, viene applicato il filtro passa basso tra il tempo che l'intero evento CPU sta impiegando per concludere e il quanto di tempo corrente.
Se quest'ultimo equivale a -1, caso del primo CPU burst di un processo, viene settato al valore di *max_quantum* poichè è stato interrotto da quest'ultimo.
Questa operazione ha come scopo quello di minimizzare le interruzioni del CPU burst quando vi sono dei notevoli aumenti da un burst all'altro.
