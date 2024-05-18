# CPU Scheduler Simulator
## Preemptive Scheduler con Shortest Job First e Quantum Prediction

L'algoritmo **`Shortest Job First`** per la scelta del processo da eseguire consiste nello scegliere il processo con il `minor CPU burst`, cioè quello che terrà meno occupata la CPU.
Il limite di questo algoritmo sta nell'_impossibilità di conoscere il futuro comportamento di un processo_ e quindi di stabilirne i tempi di CPU e IO burst.

E' possibile rimediare a questo problema cercando di prevedere il futuro comportamento del processo che, tendenzialmente, è **ciclico**: le durate dei CPU burst possono variare ma si terranno mediamente su un certo valore.

Una possibile approssimazione si ottiene applicando un _filtro passa basso_ in grado di attenuare l'effetto di burst eccezionali. Il tempo stimato del prossimo burst sarà dunque una media tra il tempo effettivamente misurato del burst corrente ed il tempo precedentemente stimato per il burst corrente:
```
B(t+1) = α * b(t) + (1-α) * B(t)
```
Il ***coefficiente di decay*** ***`α`*** serve ad attribuire nel calcolo della media un peso al ***tempo misurato*** ***`b(t)`*** o al ***tempo precedentemente stimato*** ***`B(t)`*** :
per *`α=1`* il tempo stimato del prossimo burst è pari al tempo misurato; per *`α=0`* è pari al tempo precedentemente stimato.

Nell'algoritmo implementato, si applica all'idea dello Shortest Job First il concetto di **`preemption`**: lo scheduler può togliere forzatamente la CPU ad un processo se questo la sta usando da più di un periodo di tempo chiamato **quanto** (***`cpu quantum`***). Tuttavia il kernel imposta a tempo di compilazione un **valore massimo** per il quanto, indicato con ***`max_quantum`***.

Per integrare il concetto di preemption con lo SJB, andremo a predire il prossimo CPU burst attraverso il filtro passa basso, e considereremo questo valore come il quanto di tempo dopo il quale verrà tolta la cpu al prossimo processo.

NB: il valore dei quanti di tempo calcolati rimangono tali anche se superiori a *max_quantum*, al fine di svolgere correttamente il calcolo dei successivi quanti.

Nel caso di un processo appena creato, poiché il suo primo CPU burst non ha una predizione fatta nel passato e quindi non ha ancora un quanto (*quantum=-1*), useremo come quanto di tempo la durata dell'intero evento CPU o il *`max_quantum`*, se questo è minore della durata dell'evento.

Nel caso in cui un evento di tipo CPU venga interrotto una o più volte perchè in running da un tempo maggoire o uguale al quanto (o *max_quantum*), al momento dell'interruzione (*end quantum*) l'evento verrà sostituito con un nuovo evento CPU con durata pari alla durata rimanente e quanto pari a
```
lpFilter = pcb->timer_CPU_burst * args->decay + e->quantum * (1-args->decay)
```
In altri termini, viene applicato il filtro passa basso tra il **tempo che l'intero evento CPU sta impiegando per concludere** e il **quanto di tempo corrente**.
Se quest'ultimo equivale a -1, caso del primo CPU burst di un processo, viene settato al valore di *max_quantum* poichè è stato interrotto da quest'ultimo.
Questa operazione ha come scopo quello di minimizzare le interruzioni del CPU burst quando vi sono dei notevoli aumenti da un burst all'altro.

Prendiamo ad esempio il ***processo p1*** contenuto nel file */processes/pi.txt*:

| PID:1, ARRIVAL TIME:0  | BURST |
|     :---:     |  :---: |
|   CPU BURST   |  1  |
|   IO BURST    |  1  |
|   CPU BURST   |  10  |
|   IO BURST    |  1  |
|   CPU BURST   |  7  |
|   IO BURST    |  1  |
|   CPU BURST   |  2  |
|   IO BURST    |  1  |

Introduciamo le seguenti variabili:
+ ***`e->timer`***: timer per ogni evento; viene resettato ogni volta che l'evento termina o viene interrotto.
+ ***`pcb->timer_CPU_burst`***: timer per ogni PCB; parte all'inizio di un CPU burst; viene resettato ogni volta che l'evento termina.
+ ***`e->next_pred`***: predizione della durata del prossimo CPU burst; il burst successivo avrà ***quantum*** pari a ***e->next_pred*** del burst precedente.
+ ***`pcb->last_prediction`***: viene salvato il valore dell'ultima predizione fatta per ogni CPU burst totale.

  
Supponendo che ci sia solo questo processo da eseguire, che il *coefficiente di decay* sia pari a ***`decay=0.5`*** e che ***`max_quantum=5`***, il flusso del kernel sarà:

| p1, TIME | BURST | QUANTUM | E->TIMER | PCB->TIMER_CPU_BURST | E->NEXT_PRED | PCB_LAST_PRED |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
|   0   |  CPU  |  -1   |   1   |   1   |   1   |   1   |
|   1   |  IO   |       |   1   |       |       |       |
|   2   |  CPU  |   1   |   1   |   1   |   1   |       |
|   3   |  CPU  |   1   |   1   |   2   |  1.5  |       |
|   4   |  CPU  |  1.5  |   1   |   3   |       |       |
|   5   |  CPU  |  1.5  |   2   |   4   | 2.75  |       |
|   6   |  CPU  | 2.75  |   1   |   5   |       |       |
|   7   |  CPU  | 2.75  |   2   |   6   |       |       |
|   8   |  CPU  | 2.75  |   3   |   7   | 4.88  |       |
|   9   |  CPU  | 4.88  |   1   |   8   |       |       |
|  10   |  CPU  | 4.88  |   2   |   9   |       |       |
|  11   |  CPU  | 4.88  |   3   |  10   | 7.44  |  5.5  |
|  12   |  IO   |       |   1   |       |       |       |
|  13   |  CPU  |  5.5  |   1   |   1   |       |       |
|  14   |  CPU  |  5.5  |   2   |   2   |       |       |
|  15   |  CPU  |  5.5  |   3   |   3   |       |       |
|  16   |  CPU  |  5.5  |   4   |   4   |       |       |
|  17   |  CPU  |  5.5  |   5   |   5   | 5.25  |       |
|  18   |  CPU  | 5.25  |   1   |   6   |       |       |
|  19   |  CPU  | 5.25  |   2   |   7   | 6.13  | 6.25  |
|  20   |  IO   |       |   1   |       |       |       |
|  21   |  CPU  | 6.25  |   1   |   1   |       |       |
|  21   |  CPU  | 6.25  |   2   |   2   | 4.13  | 4.13  |
|  22   |  IO   |       |   1   |       |       |       |
