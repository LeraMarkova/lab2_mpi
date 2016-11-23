#include <math.h>  
#include <stdio.h> 
#include <stdlib.h>
#include "mpi.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "mpi.h" 
#include <iostream>
using namespace std;


int main(int argc, char* argv[]) {

    //Объявление переменных
    int n = 3000;
    int ProcNum, ProcRank;
    double timepar1, timepar2, timelin1, timelin2;
    int count, ost;
    int *res;
    int *reslin;
    int *vector;
    int *matrix_trans; //транспонированная матрица(последовательно расположены столбцы)
    int **matrix;   //исходная матрица
    int *tmp;
    int *tmplong;
    int *SendCountMatrix; //сколько столбцов передать каждому процессу
    int *SendIndexMatrix; //запоминает смещение элементов матрицы транспонированной
    int *SendCountVector; //сколько элементов вектора каждому процессу
    int *SendIndexVector; //запоминает смещение элементов вектора
    int *rbufMatrix; //буфер приема матрицы всеми процессами
    int *rbufVector; //буфер приема вектора всеми процессами

                     //Начало работы с MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum); //кол-во процессов
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);//ранг процесса 

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Выделение памяти
    SendCountMatrix = new int[ProcNum];
    SendIndexMatrix = new int[ProcNum];
    SendCountVector = new int[ProcNum];
    SendIndexVector = new int[ProcNum];
    vector = new int[n];
    matrix_trans = new int[n*n];
    res = new int[n];
    reslin = new int[n];
    matrix = new int*[n];
    for (int i = 0; i<n; i++)
        matrix[i] = new int[n];

    //0-ой процесс заполняет матрицу (и транспонирует) и вектор
    if (ProcRank == 0)
    {
        for (int i = 0; i<n; i++)
            for (int j = 0; j<n; j++)
                matrix[i][j] = rand() % 10;

        int k = 0;
        for (int j = 0; j<n; j++)
            for (int i = 0; i<n; i++, k++)
                matrix_trans[k] = matrix[i][j];

        for (int i = 0; i<n; i++)
        {
            vector[i] = rand() % 10;
            res[i] = 0;
        }
    }

    // MPI_Barrier(MPI_Comm, MPI_COMM_WORLD); //для точности начала отсчета времени  

                                          //Начало отсчета парал. времени
    timepar1 = MPI_Wtime();

    //Распределение столбцов матрицы по процессам для передачи + соответсвие эл.вектора 
    count = n / ProcNum;
    ost = n%ProcNum;
    for (int i = 0; i<ost; i++)
    {
        SendCountMatrix[i] = n*(count + 1);
        SendIndexMatrix[i] = i*SendCountMatrix[i];
        SendCountVector[i] = count + 1;
        SendIndexVector[i] = i*SendCountVector[i];
    }
    for (int i = ost; i<ProcNum; i++)
    {
        SendCountMatrix[i] = n*count;
        SendIndexMatrix[i] = n*(ost + i*count);
        SendCountVector[i] = count;
        SendIndexVector[i] = ost + i*count;
    }

    //выделение соответсвующей памяти для буферов приема
    rbufMatrix = new int[SendCountMatrix[ProcRank]];
    rbufVector = new int[SendCountVector[ProcRank]];
    //Отправление соответсвующих столбцов матрицы и элементов вектора процессам
    MPI_Scatterv(matrix_trans, SendCountMatrix, SendIndexMatrix, MPI_INT, rbufMatrix, SendCountMatrix[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(vector, SendCountVector, SendIndexVector, MPI_INT, rbufVector, SendCountVector[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);

    tmplong = new int[SendCountMatrix[ProcRank]];
    tmp = new int[n];
    //Обнуляем tmp, tmp long
    for (int i = 0; i<n; i++)
        tmp[i] = 0;
    for (int i = 0; i<SendCountMatrix[ProcRank]; i++)
        tmplong[i] = 0;
    //Считаем tmp
    int k = 0;
    for (int i = 0; i<SendCountVector[ProcRank]; i++)
        for (int j = 0; j<n; j++, k++)
            tmplong[k] = rbufMatrix[j + i*n] * rbufVector[i];
    if (SendCountMatrix[ProcRank] == n)
        tmp = tmplong;
    if (SendCountMatrix[ProcRank]>n)
    {
        for (int i = 0; i<n; i++)
            for (int j = 0; j<SendCountVector[ProcRank]; j++)
            {
                tmp[i] += tmplong[i + j*n];
            }
    }

    //Отправляемый полученный tmp 0-му процессу от всех процессов
    MPI_Reduce(tmp, res, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  //  MPI_Barrier(MPI_Comm, MPI_COMM_WORLD); //для точности конца отсчета времени
    timepar2 = MPI_Wtime();

    //Лин. часть программы + выводы

   /* if (ProcRank == 0)
    {
        cout << endl;
        cout << "Source Matrix" << endl;
        for (int i = 0; i<n; i++) //столбцы
        {
            for (int j = 0; j<n; j++) //строки
                printf("%3d ", matrix[i][j]);
            printf("\n");
        }

        cout << endl;

        cout << "Source Vector" << endl;
        for (int z = 0; z < n; z++)
        {
            printf("%3d ", vector[z]);
            printf("\n");
        }

        cout << endl; 

        cout << "Parallel Result" << endl;
        for (int p = 0; p < n; p++)
        {
        printf("%3d ", tmp[p]);
        printf("\n");
        }   

    }*/

    if (ProcRank == 0)
    {


        for (int i = 0; i<n; i++)
            reslin[i] = 0;

        timelin1 = MPI_Wtime();

        for (int j = 0; j<n; j++)
            for (int i = 0; i<n; i++)
            {
                reslin[i] += matrix[i][j] * vector[j];
               
            }
        
        timelin2 = MPI_Wtime();
       /* cout << "result lin" << endl;
        for (int i = 0; i < n; i++)
        {
            printf("%3d ", reslin[i]);
            printf("\n");
        }*/
       
        //Проверка корректности + вывод результата
        int prov = 0;
        for (int i = 0; i<n; i++)
            if (res[i] != reslin[i])
                prov++;
        if (prov == 0)
            cout << "Correct" << endl;
        else
            cout << "Uncorrect" << endl;
        cout << "Lin Time = " << timelin2 - timelin1 << endl;
       
        cout << "Parral Time = " << timepar2 - timepar1 << endl;
    }
    //Завершаем работу с MPI
    MPI_Finalize();

    //Освобождаем память
    delete res;
    delete reslin;
    delete vector;
    delete matrix_trans;
    delete tmp;
    delete tmplong;
    delete SendCountMatrix;
    delete SendIndexMatrix;
    delete SendCountVector;
    delete SendIndexVector;
    delete rbufMatrix;
    delete rbufVector;
    for (int i = 0; i<n; i++)
        delete matrix[i];

    return 0;
}
