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

    //���������� ����������
    int n = 3000;
    int ProcNum, ProcRank;
    double timepar1, timepar2, timelin1, timelin2;
    int count, ost;
    int *res;
    int *reslin;
    int *vector;
    int *matrix_trans; //����������������� �������(��������������� ����������� �������)
    int **matrix;   //�������� �������
    int *tmp;
    int *tmplong;
    int *SendCountMatrix; //������� �������� �������� ������� ��������
    int *SendIndexMatrix; //���������� �������� ��������� ������� �����������������
    int *SendCountVector; //������� ��������� ������� ������� ��������
    int *SendIndexVector; //���������� �������� ��������� �������
    int *rbufMatrix; //����� ������ ������� ����� ����������
    int *rbufVector; //����� ������ ������� ����� ����������

                     //������ ������ � MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum); //���-�� ���������
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);//���� �������� 

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //��������� ������
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

    //0-�� ������� ��������� ������� (� �������������) � ������
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

    // MPI_Barrier(MPI_Comm, MPI_COMM_WORLD); //��� �������� ������ ������� �������  

                                          //������ ������� �����. �������
    timepar1 = MPI_Wtime();

    //������������� �������� ������� �� ��������� ��� �������� + ����������� ��.������� 
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

    //��������� �������������� ������ ��� ������� ������
    rbufMatrix = new int[SendCountMatrix[ProcRank]];
    rbufVector = new int[SendCountVector[ProcRank]];
    //����������� �������������� �������� ������� � ��������� ������� ���������
    MPI_Scatterv(matrix_trans, SendCountMatrix, SendIndexMatrix, MPI_INT, rbufMatrix, SendCountMatrix[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(vector, SendCountVector, SendIndexVector, MPI_INT, rbufVector, SendCountVector[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);

    tmplong = new int[SendCountMatrix[ProcRank]];
    tmp = new int[n];
    //�������� tmp, tmp long
    for (int i = 0; i<n; i++)
        tmp[i] = 0;
    for (int i = 0; i<SendCountMatrix[ProcRank]; i++)
        tmplong[i] = 0;
    //������� tmp
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

    //������������ ���������� tmp 0-�� �������� �� ���� ���������
    MPI_Reduce(tmp, res, n, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  //  MPI_Barrier(MPI_Comm, MPI_COMM_WORLD); //��� �������� ����� ������� �������
    timepar2 = MPI_Wtime();

    //���. ����� ��������� + ������

   /* if (ProcRank == 0)
    {
        cout << endl;
        cout << "Source Matrix" << endl;
        for (int i = 0; i<n; i++) //�������
        {
            for (int j = 0; j<n; j++) //������
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
       
        //�������� ������������ + ����� ����������
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
    //��������� ������ � MPI
    MPI_Finalize();

    //����������� ������
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
