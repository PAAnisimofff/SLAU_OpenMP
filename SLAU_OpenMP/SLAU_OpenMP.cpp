// SLAU_OpenMP.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include <iostream>
#include <ctime>
#include <omp.h>
#include <vector>
using namespace std;



vector <vector<double>> search_reverse_matrix_sequential(vector <vector<double>> matrix)
{
	int size = matrix.size();
	vector <vector<double>> E(size, vector<double>(size));

	//Заполнение единичной матрицы
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			if (i == j) E[i][j] = 1.0;
			else E[i][j] = 0.0;
		}
	}

	//Трансформация исходной матрицы в верхнетреугольную
	for (int k = 0; k < size; k++)
	{
		if (abs(matrix[k][k]) < 1e-8)
		{
			bool changed = false;
			for (int i = k + 1; i < size; i++)
			{
				if (abs(matrix[i][k]) > 1e-8)
				{
					swap(matrix[k], matrix[i]);
					swap(E[k], E[i]);
					changed = true;
					break;
				}
			}
			if (!changed)
			{
				cout << "\nНевозможно найти обратную матрицу!\n";
				exit(-1);
			}
		}

		double div = matrix[k][k];
		//Распараллеленный цикл деления элементов
			for (int j = 0; j < size; j++)
			{
				matrix[k][j] /= div;
				E[k][j] /= div;
			}
		//Распараллеленный цикл зануления элементов в процессе прямого хода
			for (int i = k + 1; i < size; i++)
			{
				double multi = matrix[i][k];
				for (int j = 0; j < size; j++)
				{
					matrix[i][j] -= multi * matrix[k][j];
					E[i][j] -= multi * E[k][j];
				}
			}
	}

	//Формирование единичной матрицы из исходной
	//и обратной из единичной
	for (int k = size - 1; k > 0; k--)
	{
		//Распараллеленный цикл зануления элементов в процессе прямого хода
			for (int i = k - 1; i > -1; i--)
			{
				double multi = matrix[i][k];

				for (int j = 0; j < size; j++)
				{
					matrix[i][j] -= multi * matrix[k][j];
					E[i][j] -= multi * E[k][j];
				}
			}
	}
	matrix = E;
	return matrix;
	//return true;
}









vector <vector<double>> search_reverse_matrix(vector <vector<double>> matrix)
{
	int size = matrix.size();
	vector <vector<double>> E(size, vector<double>(size));
	int cores = 16;
	//Заполнение единичной матрицы
#pragma omp parallel num_threads(cores)
	{
#pragma omp for
		for (int i = 0; i < size; i++)
			for (int j = 0; j < size; j++)
			{
				if (i == j)
					E[i][j] = 1.0;
				else
					E[i][j] = 0.0;
			}
	}
	//for (int i = 0; i < size; i++)
	//{
	//	for (int j = 0; j < size; j++)
	//	{
	//		if (i == j) E[i][j] = 1.0;
	//		else E[i][j] = 0.0;
	//	}
	//}

	//Трансформация исходной матрицы в верхнетреугольную
	for (int k = 0; k < size; k++)
	{
		if (abs(matrix[k][k]) < 1e-8)
		{
			bool changed = false;

			for (int i = k + 1; i < size; i++)
			{
				if (abs(matrix[i][k]) > 1e-8)
				{
					swap(matrix[k], matrix[i]);
					swap(E[k], E[i]);
					changed = true;
					break;
				}
			}

			if (!changed)
			{
				cout << "\nНевозможно найти обратную матрицу!\n";
				exit(-1);
			}
		}

		double div = matrix[k][k];
		//Распараллеленный цикл деления элементов
#pragma omp parallel num_threads(cores)
		{
#pragma omp for
			for (int j = 0; j < size; j++)
			{
				matrix[k][j] /= div;
				E[k][j] /= div;
			}
		}
		//Распараллеленный цикл зануления элементов в процессе прямого хода
#pragma omp parallel num_threads(cores)
		{
#pragma omp for
			for (int i = k + 1; i < size; i++)
			{
				double multi = matrix[i][k];


				for (int j = 0; j < size; j++)
				{
					matrix[i][j] -= multi * matrix[k][j];
					E[i][j] -= multi * E[k][j];
				}
			}
		}
	}

	//Формирование единичной матрицы из исходной
	//и обратной из единичной
	for (int k = size - 1; k > 0; k--)
	{
		//Распараллеленный цикл зануления элементов в процессе прямого хода
#pragma omp parallel num_threads(cores)
		{
#pragma omp for
			for (int i = k - 1; i > -1; i--)
			{
				double multi = matrix[i][k];

				for (int j = 0; j < size; j++)
				{
					matrix[i][j] -= multi * matrix[k][j];
					E[i][j] -= multi * E[k][j];
				}
			}
		}
	}
	matrix = E;
	return matrix;
	//return true;
}

double random(const int min, const int max)
{
	if (min == max)
		return min;
	return min + rand() % (max - min);
}

vector<double> parallel_result(vector <vector<double>> pMatrix, vector<double> pVector, int Size)
{
	int i, j;
	vector<double> pResult(Size);
#pragma omp parallel for private (j) //schedule(dynamic, Size / omp_get_max_threads()) 
	for (i = 0; i < Size; i++)
	{
		pResult[i] = 0;
		for (j = 0; j < Size; j++)
			pResult[i] += pMatrix[i][j] * pVector[j];
	}
	return pResult;
}


int main()
{
	setlocale(LC_ALL, "RUS");
	int equations_amount;
	cout << "Kол-во ур-ий\tОшибка параллельная\tОшибка последовательная\tВремя параллельное (c)\tВремя последовательное (c)\n";
	for (equations_amount = 10; equations_amount <= 640; equations_amount *= 2)
	{
		//cout << "Kоличество уравнений: " << equations_amount << endl;

		vector<vector<double>> matrix(equations_amount, vector<double>(equations_amount));
		vector<double> B(equations_amount);

		// Заполняем матрицу коэффициентов и B
		for (int i = 0; i < equations_amount; i++)
		{
			for (int j = 0; j < equations_amount; j++)
				matrix[i][j] = random(0, 100);
			B[i] = random(0, 100);
		}

		// Вывод системы уравнений
		/*cout << "\nСистема уравнений:\n";
		for (int i = 0; i < equations_amount; i++)
		{
			for (int j = 0; j < equations_amount; j++)
			{

				if (j != 0 && matrix[i][j] >= 0)
					cout << " +";
				cout << " " << matrix[i][j] << "x" << j + 1;
			}
			cout << " = " << B[i] << endl;
		}*/

		//vector<vector<double>> orig_matrix = matrix;
		double t_s = clock();
		vector<vector<double>> s_reverse_matrix = search_reverse_matrix_sequential(matrix);
		vector<double> X_S(equations_amount);
		for (int i = 0; i < equations_amount; i++)
		{
			X_S[i] = 0;
			for (int j = 0; j < equations_amount; j++)
				X_S[i] += s_reverse_matrix[i][j] * B[j];
		}
		t_s = (clock() - t_s) / 1000;
		double t = clock();
		vector<vector<double>> reverse_matrix = search_reverse_matrix(matrix);
		// Вычисление обратной матрицы
		/*if (!search_reverse_matrix(matrix))
		{
			cout << "\nНевозможно найти обратную матрицу!\n";
			exit(1);
		}*/

		// Матрица-столбец неизвестных X и вычисление окончательного результата
		vector<double> X(equations_amount);
		//Распараллеленный цикл вычисления неизвестных переменных
		int cores = 16;
#pragma omp parallel num_threads(cores)
		{
#pragma omp for
			for (int i = 0; i < equations_amount; i++)
			{
				X[i] = 0;
				for (int j = 0; j < equations_amount; j++)
					X[i] += reverse_matrix[i][j] * B[j];
			}
		}

		// Вывод окончательного результата
		/*cout << "\nРешение системы уравнений:";
		for (int i = 0; i < equations_amount; i++)
			cout << "\nx" << i + 1 << " = " << X[i];*/

		t = (clock() - t) / 1000;
		vector<double> check = parallel_result(matrix, X, equations_amount);
		vector<double> check_s = parallel_result(matrix, X_S, equations_amount);
		double sum = 0, sum_s = 0;
		//cout << "\nПроверка:\n";
		for (int i = 0; i < equations_amount; i++)
		{
			sum += abs(B[i] - check[i]);
			sum_s += abs(B[i] - check_s[i]);
		}
		cout << equations_amount << "\t" << sum / equations_amount << "\t" << sum_s / equations_amount << "\t" << t << "\t" << t_s << endl;
		//cout << "Kоличество уравнений: " << equations_amount << endl;
		//cout << "\nОшибка: " << sum / equations_amount;
		//cout << "\n\nВремя, затраченное на вычисление: " << t << "с.\n";
	}
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
