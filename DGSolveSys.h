#ifndef TRI_DGSOLVESYS_H
#define TRI_DGSOLVESYS_H

#define __DGSOLVESYS_DEBUG
// #define __DGSOLVESYS_DEBUG_EDGE
// #define __DGSOLVESYS_DEBUG_LV2

#include "solveSys.h"
#include "DGProblem.h"

typedef std::vector< std::vector<double> > VECMATRIX;

template <typename MyProblem>
class DGSolvingSystem:public BasicSolvingSystem<MyProblem>
{
	const int LocalDimension = 3;
	double penaltyOver3, penaltyOver6;
	
	void calcDetBE(Mesh<MyProblem> &mesh);

	std::vector< std::vector<double> > elementInteg(Element ele, Mesh<MyProblem> mesh);

	std::vector<double> elementIntegRhs(Element ele, Mesh<MyProblem> mesh, MyProblem prob);

	int edgeInteg(Edge edge, Mesh<MyProblem> mesh, MyProblem prob, VECMATRIX &M11, VECMATRIX &M12, VECMATRIX &M21, VECMATRIX &M22);

	int edgeInteg(Edge edge, Mesh<MyProblem> mesh, MyProblem prob, VECMATRIX &M11);

	int assembleElement(Element ele, Mesh<MyProblem> mesh, MyProblem prob);

	int assembleEdge(Edge edge, Mesh<MyProblem> mesh, MyProblem prob);

	double innerProduct(std::vector<double> x, std::vector<double> y);
	
	double penaltyTerm(Edge edge, int v1, int v2);
	
	int calcNormalVectorAndIntegOnEdge(Edge edge, Mesh<MyProblem> mesh, std::vector<double> &ne,
		std::vector<double> &integ_e);

	int calcNormalVectorAndIntegOnEdge(Edge edge, Mesh<MyProblem> mesh, std::vector<double> &ne,
		std::vector<double> &integ_e1, std::vector<double> &integ_e2);

	int getMii(Mesh<MyProblem> mesh, Edge edge, VECMATRIX &M, Element E1, Element E2, std::vector<double> integ_e1, std::vector<double> integ_e2, double eps,
		std::vector<double> ne, std::vector< std::vector<double> > grad_E1, std::vector< std::vector<double> > grad_E2, int sign1, int sing2, int sing3);

	int consoleOutput(Mesh<MyProblem> mesh);

	int fileOutput(Mesh<MyProblem> mesh, MyProblem prob);

public:

	int assembleStiff(Mesh<MyProblem> &mesh, MyProblem prob);

	int triOutput(MyProblem prob, Mesh<MyProblem> mesh);
};


using std::vector;
using std::cout;
using std::endl;

template <typename MyProblem>
void DGSolvingSystem<MyProblem>::calcDetBE(Mesh<MyProblem> &mesh)
{
	for(Element &ele : mesh.element){
		double x1(mesh.vertex[ele.vertex[0] - 1].x), y1(mesh.vertex[ele.vertex[0] - 1].y),
		   x2(mesh.vertex[ele.vertex[1] - 1].x), y2(mesh.vertex[ele.vertex[1] - 1].y),
		   x3(mesh.vertex[ele.vertex[2] - 1].x), y3(mesh.vertex[ele.vertex[2] - 1].y);

		ele.detBE = fabs((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)); // absolute value needed here?
	}
}

template <typename MyProblem>
vector< vector<double> > DGSolvingSystem<MyProblem>::elementInteg(Element ele, Mesh<MyProblem> mesh)
{
	vector< vector<double> > vecElementInteg;
	vecElementInteg.resize(ele.localDof);

	#ifdef __DGSOLVESYS_DEBUG_LV2
				cout << "  local dof = " << ele.localDof << endl;
	#endif
	
	double x1(mesh.vertex[ele.vertex[0] - 1].x), y1(mesh.vertex[ele.vertex[0] - 1].y),
		   x2(mesh.vertex[ele.vertex[1] - 1].x), y2(mesh.vertex[ele.vertex[1] - 1].y),
		   x3(mesh.vertex[ele.vertex[2] - 1].x), y3(mesh.vertex[ele.vertex[2] - 1].y);

	double rec_2detBE = 0.5 / ele.detBE;
	double detBE_over_12 = ele.detBE / 12.0;
	double detBE_over_24 = ele.detBE / 24.0;

	for(int vi = 0, row = 0; vi < LocalDimension; ++vi)
	{
		if(mesh.vertex[ele.vertex[vi] - 1].bctype > 0)
			continue;
		
		for(int vj = vi, col = row; vj < LocalDimension; ++vj) // only calculate the upper triangle part of the matrix
		{
			if(mesh.vertex[ele.vertex[vj] - 1].bctype > 0)
				continue;

			double tempInteg;
			if(vi == 0){
				switch(vj){
					case 0:{tempInteg = ((y2 - y3) * (y2 - y3) + (x3 - x2) * (x3 - x2)) * rec_2detBE + detBE_over_12; break;}
					case 1:{tempInteg = ((y2 - y3) * (y3 - y1) + (x3 - x2) * (x1 - x3)) * rec_2detBE + detBE_over_24; break;}
					case 2:{tempInteg = ((y2 - y3) * (y1 - y2) + (x3 - x2) * (x2 - x1)) * rec_2detBE + detBE_over_24; break;}
				}
			}
			else if(vi == 1){
				switch(vj){
					case 1:{tempInteg = ((y3 - y1) * (y3 - y1) + (x1 - x3) * (x1 - x3)) * rec_2detBE + detBE_over_12; break;}
					case 2:{tempInteg = ((y3 - y1) * (y1 - y2) + (x1 - x3) * (x2 - x1)) * rec_2detBE + detBE_over_24; break;}
				}
			}
			else{
				tempInteg = ((y1 - y2) * (y1 - y2) + (x2 - x1) * (x2 - x1)) * rec_2detBE + detBE_over_12;
			}

			vecElementInteg[row].push_back(tempInteg);
			if(row != col)
				vecElementInteg[col].push_back(tempInteg); // because of symmetry

	#ifdef __DGSOLVESYS_DEBUG_LV2
				cout << "  global " << ele.vertex[vi] << " local " << vi
						  << " and global " << ele.vertex[vj] << " local " << vj
						  << " integ = " << tempInteg << endl;
	#endif
			++col;
		}
		++row;
	}

	return vecElementInteg;
}

template <typename MyProblem>
vector<double> DGSolvingSystem<MyProblem>::elementIntegRhs(Element ele, Mesh<MyProblem> mesh, MyProblem prob)
{
	vector<double> vecElementIntegRhs;
	
	double x1(mesh.vertex[ele.vertex[0] - 1].x), y1(mesh.vertex[ele.vertex[0] - 1].y),
		   x2(mesh.vertex[ele.vertex[1] - 1].x), y2(mesh.vertex[ele.vertex[1] - 1].y),
		   x3(mesh.vertex[ele.vertex[2] - 1].x), y3(mesh.vertex[ele.vertex[2] - 1].y);

	vector<double> F(LocalDimension, 0);
	
	F[0] = prob.f((x2 + x3) / 2.0, (y2 + y3) / 2.0);
	F[1] = prob.f((x1 + x3) / 2.0, (y1 + y3) / 2.0);
	F[2] = prob.f((x1 + x2) / 2.0, (y1 + y2) / 2.0);

	double detBE_over_12 = ele.detBE / 12.0;

	for(int vi = 0; vi < LocalDimension; ++vi){
		if(mesh.vertex[ele.vertex[vi] - 1].bctype > 0)
			continue;

		double a = 0;
		for(int i = 0; i < ele.vertex.size(); i++){
			if (i == vi)
				continue;
			a += F[i];
		}

		a *= detBE_over_12;
		vecElementIntegRhs.push_back(a);

	#ifdef __DGSOLVESYS_DEBUG_LV2
			cout << "  global " << ele.vertex[vi] << " local " << vi
					  << " rh = " << a << endl;
	#endif
	}

	return vecElementIntegRhs;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::assembleElement(Element ele, Mesh<MyProblem> mesh, MyProblem prob)
{

	vector< vector<double> > vecElementInteg = elementInteg(ele, mesh);
	
	for(int row = 0; row != vecElementInteg.size(); ++row)
		for(int col = 0; col != vecElementInteg[row].size(); ++col){

	#ifdef __DGSOLVESYS_DEBUG_LV2
				cout << "  add to " << ele.index + row << ", " << ele.index + col << endl;
	#endif
			this -> addToMA(vecElementInteg[row][col], ele.index + row, ele.index + col);
		}

	// why commenting off this part causes a segmentation fault?
	vector<double> vecElementIntegRhs;
	vecElementIntegRhs = elementIntegRhs(ele, mesh, prob);
	for(int i = 0; i != vecElementIntegRhs.size(); ++i){

	#ifdef __DGSOLVESYS_DEBUG_LV2
			cout << "  add rh to " << ele.index + i << endl;
	#endif
		this -> rh[ele.index + i] += vecElementIntegRhs[i];
	}

	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::calcNormalVectorAndIntegOnEdge(Edge edge, Mesh<MyProblem> mesh, vector<double> &ne,
	vector<double> &integ_e1, vector<double> &integ_e2)
{
	if(edge.neighborElement.size() != 2){
		cout << "invalid call" << endl;
		return 1;
	}

	Element &E1 = mesh.element[ edge.neighborElement[0] - 1];
	Element &E2 = mesh.element[ edge.neighborElement[1] - 1];

	double x1, x2, x3, y1, y2, y3;
	
	vector<double> integ_e1_tmp(LocalDimension, 1);
	vector<double> integ_e2_tmp(LocalDimension, 1);
	
	int k = -1;
	for(int ver:E1.vertex)
	{
		++k;
		if(edge.vertex[0] == ver){
			continue;
		}
		if(edge.vertex[1] == ver){
			continue;
		}
		x1 = mesh.vertex[ver - 1].x;
		y1 = mesh.vertex[ver - 1].y;

		integ_e1_tmp[k] = 0;
	}
	
	k = -1;
	for(int ver:E2.vertex)
	{
		++k;
		if(edge.vertex[0] == ver){
			continue;
		}
		if(edge.vertex[1] == ver){
			continue;
		}
		integ_e2_tmp[k] = 0;
	}

	x2 = mesh.vertex[edge.vertex[0] - 1].x;
	y2 = mesh.vertex[edge.vertex[0] - 1].y;
	x3 = mesh.vertex[edge.vertex[1] - 1].x;
	y3 = mesh.vertex[edge.vertex[1] - 1].y;

	if( ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) < 0 )
	{
		std::swap(x2, x3);
		std::swap(y2, y3);
	}

	ne.clear();
	ne.push_back( (y3 - y2) / 4.0 );
	ne.push_back( (x2 - x3) / 4.0 );

	integ_e1.clear();
	integ_e1 = integ_e1_tmp;
	integ_e2.clear();
	integ_e2 = integ_e2_tmp;

	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::calcNormalVectorAndIntegOnEdge(Edge edge, Mesh<MyProblem> mesh, vector<double> &ne,
	vector<double> &integ_e)
{
	if(edge.neighborElement.size() != 1){
		cout << "invalid call" << endl;
		return 1;
	}

	Element &ele = mesh.element[ edge.neighborElement[0] - 1];
	double x1, x2, x3, y1, y2, y3;
	
	vector<double> integ_e_tmp(LocalDimension, 1);
	
	int k = -1;
	for(int ver:ele.vertex)
	{
		++k;
		if(edge.vertex[0] == ver){
			continue;
		}
		if(edge.vertex[1] == ver){
			continue;
		}
		x1 = mesh.vertex[ver - 1].x;
		y1 = mesh.vertex[ver - 1].y;

		integ_e_tmp[k] = 0;
	}

	x2 = mesh.vertex[edge.vertex[0] - 1].x;
	y2 = mesh.vertex[edge.vertex[0] - 1].y;
	x3 = mesh.vertex[edge.vertex[1] - 1].x;
	y3 = mesh.vertex[edge.vertex[1] - 1].y;

	if( ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) < 0 )
	{
		std::swap(x2, x3);
		std::swap(y2, y3);
	}

	ne.clear();
	ne.push_back( (y3 - y2) / 2.0 );
	ne.push_back( (x2 - x3) / 2.0 );

	integ_e = integ_e_tmp;

	return 0;
}

template <typename MyProblem>
double DGSolvingSystem<MyProblem>::innerProduct(vector<double> x, vector<double> y)
{
	return x[0] * y[0] + x[1] * y[1];
}

void initM(VECMATRIX &M11, VECMATRIX &M12, VECMATRIX &M21, VECMATRIX &M22, int dofE1, int dofE2)
{
	M11.resize(dofE1);
	for(int i = 0; i != dofE1; ++i)
		M11[i].resize(dofE1);

	M12.resize(dofE1);
	for(int i = 0; i != dofE1; ++i)
		M12[i].resize(dofE2);

	M21.resize(dofE2);
	for(int i = 0; i != dofE2; ++i)
		M21[i].resize(dofE1);

	M22.resize(dofE2);
	for(int i = 0; i != dofE2; ++i)
		M22[i].resize(dofE2);
}

void initM(VECMATRIX &M11, int dofE1)
{
	M11.resize(dofE1);
	for(int i = 0; i != dofE1; ++i)
		M11[i].resize(dofE1);
}

template <typename MyProblem>
double DGSolvingSystem<MyProblem>::penaltyTerm(Edge edge, int v1, int v2)
{
	// if any of vi, v2 is not on the edge, then the penalty term is 0
	if((v1 != edge.vertex[0] && v1 != edge.vertex[1]) || (v2 != edge.vertex[0] && v2 != edge.vertex[1]))
		return 0;

	if(v1 == v2)
		return penaltyOver3; // i.e. sigma0 / 3.0
	else
		return penaltyOver6; // i.e. sigma0 / 6.0
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::getMii(Mesh<MyProblem> mesh, Edge edge, VECMATRIX &M, Element E1, Element E2, vector<double> integ_e1, vector<double> integ_e2, double eps,
	vector<double> ne, vector< vector<double> > grad_E1, vector< vector<double> > grad_E2, int sign1, int sign2, int sign3)
{
	int row(0), col(0);
	for(int iver = 0; iver != E1.vertex.size(); ++iver){
		if(mesh.vertex[ E1.vertex[iver] - 1 ].bctype > 0)
			continue;
		col = 0;
		for(int jver = 0; jver != E2.vertex.size(); ++jver)
		{
			if(mesh.vertex[ E2.vertex[jver] - 1 ].bctype > 0)
				continue;
			M[row][col] = sign1 * integ_e1[iver] * innerProduct(grad_E2[jver], ne) + sign2 * eps * integ_e2[jver] * innerProduct(grad_E1[iver], ne);
			M[row][col] += sign3 * penaltyTerm(edge, E2.vertex[jver], E1.vertex[iver]);
			++col;
		}
		++row;
	}

	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::edgeInteg(Edge edge, Mesh<MyProblem> mesh, MyProblem prob, VECMATRIX &M11, VECMATRIX &M12, VECMATRIX &M21, VECMATRIX &M22)
{
	if(edge.neighborElement.size() != 2){
		cout << "invalid call" << endl;
		return 1;
	}

	Element &E1 = mesh.element[edge.neighborElement[0] - 1];
	Element &E2 = mesh.element[edge.neighborElement[1] - 1];

	double E1_x1(mesh.vertex[ E1.vertex[0] -1].x),
		   E1_x2(mesh.vertex[ E1.vertex[1] -1].x),
		   E1_x3(mesh.vertex[ E1.vertex[2] -1].x),
		   E1_y1(mesh.vertex[ E1.vertex[0] -1].y),
		   E1_y2(mesh.vertex[ E1.vertex[1] -1].y),
		   E1_y3(mesh.vertex[ E1.vertex[2] -1].y);
	double E2_x1(mesh.vertex[ E2.vertex[0] -1].x),
		   E2_x2(mesh.vertex[ E2.vertex[1] -1].x),
		   E2_x3(mesh.vertex[ E2.vertex[2] -1].x),
		   E2_y1(mesh.vertex[ E2.vertex[0] -1].y),
		   E2_y2(mesh.vertex[ E2.vertex[1] -1].y),
		   E2_y3(mesh.vertex[ E2.vertex[2] -1].y);

	double rec_detBE1 = 1.0 / E1.detBE;	
	double rec_detBE2 = 1.0 / E2.detBE;
	
	vector< vector<double> > grad_E1(LocalDimension);
	grad_E1[0].push_back((E1_y2 - E1_y3) * rec_detBE1);
	grad_E1[0].push_back((E1_x3 - E1_x2) * rec_detBE1);
	grad_E1[1].push_back((E1_y3 - E1_y1) * rec_detBE1);	
	grad_E1[1].push_back((E1_x1 - E1_x3) * rec_detBE1);
	grad_E1[2].push_back((E1_y1 - E1_y2) * rec_detBE1);	
	grad_E1[2].push_back((E1_x2 - E1_x1) * rec_detBE1);

	vector< vector<double> > grad_E2(LocalDimension);
	grad_E2[0].push_back((E2_y2 - E2_y3) * rec_detBE2);
	grad_E2[0].push_back((E2_x3 - E2_x2) * rec_detBE2);
	grad_E2[1].push_back((E2_y3 - E2_y1) * rec_detBE2);	
	grad_E2[1].push_back((E2_x1 - E2_x3) * rec_detBE2);
	grad_E2[2].push_back((E2_y1 - E2_y2) * rec_detBE2);	
	grad_E2[2].push_back((E2_x2 - E2_x1) * rec_detBE2);

	vector<double> ne;
	vector<double> integ_e1, integ_e2;	
	calcNormalVectorAndIntegOnEdge(edge, mesh, ne, integ_e1, integ_e2); // normal vector, not unit, actualy |e| / 4 * n_e
	
	#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  normal vector from element " << edge.neighborElement[0]
				 << " to element " << edge.neighborElement[1] << " = ("
				 << ne[0] << ", " << ne[1] << ")" << endl;
	#endif
	#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  vertices of E1 = (" << E1.vertex[0]
				 << ", " << E1.vertex[1] << ", "
				 << E1.vertex[2] << "), integ on edge = ("
				 << integ_e1[0] << ", " << integ_e1[1]
				 << ", " << integ_e1[2] << ")" << endl;
	#endif
	
	#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  vertices of E2 = (" << E2.vertex[0]
				 << ", " << E2.vertex[1] << ", "
				 << E2.vertex[2] << "), integ on edge = ("
				 << integ_e2[0] << ", " << integ_e2[1]
				 << ", " << integ_e2[2] << ")" << endl;
	#endif

	initM(M11, M12, M21, M22, E1.localDof, E2.localDof);

	const double eps = prob.epsilon;
	
	getMii(mesh, edge, M11, E1, E1, integ_e1, integ_e1, eps, ne, grad_E1, grad_E1, -1,  1,  1);
	getMii(mesh, edge, M12, E1, E2, integ_e1, integ_e2, eps, ne, grad_E1, grad_E2, -1, -1, -1);
	getMii(mesh, edge, M21, E2, E1, integ_e2, integ_e1, eps, ne, grad_E2, grad_E1,  1,  1, -1);
	getMii(mesh, edge, M22, E2, E2, integ_e2, integ_e2, eps, ne, grad_E2, grad_E2,  1, -1,  1);


	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::edgeInteg(Edge edge, Mesh<MyProblem> mesh, MyProblem prob, VECMATRIX &M11)
{
	if(edge.neighborElement.size() != 1){
		cout << "invalid call" << endl;
		return 1;
	}

	Element &E1 = mesh.element[edge.neighborElement[0] - 1];

	double E1_x1(mesh.vertex[ E1.vertex[0] -1].x),
		   E1_x2(mesh.vertex[ E1.vertex[1] -1].x),
		   E1_x3(mesh.vertex[ E1.vertex[2] -1].x),
		   E1_y1(mesh.vertex[ E1.vertex[0] -1].y),
		   E1_y2(mesh.vertex[ E1.vertex[1] -1].y),
		   E1_y3(mesh.vertex[ E1.vertex[2] -1].y);

	double rec_detBE1 = 1.0 / E1.detBE;	
	
	vector< vector<double> > grad_E1(LocalDimension);
	grad_E1[0].push_back((E1_y2 - E1_y3) * rec_detBE1);
	grad_E1[0].push_back((E1_x3 - E1_x2) * rec_detBE1);
	grad_E1[1].push_back((E1_y3 - E1_y1) * rec_detBE1);	
	grad_E1[1].push_back((E1_x1 - E1_x3) * rec_detBE1);
	grad_E1[2].push_back((E1_y1 - E1_y2) * rec_detBE1);	
	grad_E1[2].push_back((E1_x2 - E1_x1) * rec_detBE1);

	vector<double> ne;
	vector<double> integ_e1;	
	calcNormalVectorAndIntegOnEdge(edge, mesh, ne, integ_e1); // normal vector, not unit, actualy |e| / 2 * n_e

	#ifdef __DGSOLVESYS_DEBUG_EDGE
		cout << "  normal vector on boundary edge " << edge.index
			 << " = (" << ne[0] << ", " << ne[1] << ")" << endl;
	#endif

	#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  vertices of E1 = (" << E1.vertex[0]
				 << ", " << E1.vertex[1] << ", "
				 << E1.vertex[2] << "), integ on edge = ("
				 << integ_e1[0] << ", " << integ_e1[1]
				 << ", " << integ_e1[2] << ")" << endl;
	#endif

	initM(M11, E1.localDof);
	const double eps = prob.epsilon;
	// const double penalty = prob.sigma0;
	// M11

	getMii(mesh, edge, M11, E1, E1, integ_e1, integ_e1, eps, ne, grad_E1, grad_E1, -1, 1, 1);

	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::assembleEdge(Edge edge, Mesh<MyProblem> mesh, MyProblem prob)
{
	VECMATRIX M11, M12, M21, M22;
	Element &E1 = mesh.element[edge.neighborElement[0] - 1];

	if(edge.neighborElement.size() == 2){
		Element &E2 = mesh.element[edge.neighborElement[1] - 1];

		edgeInteg(edge, mesh, prob, M11, M12, M21, M22);
		
		// M11
		#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  M11" << endl;
		#endif
		for(int row = 0; row != M11.size(); ++row)
			for(int col = 0; col != M11[row].size(); ++col){
				this -> addToMA(M11[row][col], E1.index + row, E1.index + col);
			#ifdef __DGSOLVESYS_DEBUG_EDGE
					cout << "   ( " << E1.index + row
						 << " , " << E1.index + col << ") = "
						 << M11[row][col] << endl;
			#endif

			}

		// M12
		#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  M12" << endl;
		#endif
		for(int row = 0; row != M12.size(); ++row)
			for(int col = 0; col != M12[row].size(); ++col){
				this -> addToMA(M12[row][col], E1.index + row, E2.index + col);
			#ifdef __DGSOLVESYS_DEBUG_EDGE
					cout << "   ( " << E1.index + row
						 << " , " << E2.index + col << ") = "
						 << M12[row][col] << endl;
			#endif
			}

		// M21
		#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  M21" << endl;
		#endif
		for(int row = 0; row != M21.size(); ++row)
			for(int col = 0; col != M21[row].size(); ++col){
				this -> addToMA(M21[row][col], E2.index + row, E1.index + col);
			#ifdef __DGSOLVESYS_DEBUG_EDGE
					cout << "   ( " << E2.index + row
						 << " , " << E1.index + col << ") = "
						 << M21[row][col] << endl;
			#endif
			}
		
		// M22
		#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  M22" << endl;
		#endif
		for(int row = 0; row != M22.size(); ++row)
			for(int col = 0; col != M22[row].size(); ++col){
				this -> addToMA(M22[row][col], E2.index + row, E2.index + col);
			#ifdef __DGSOLVESYS_DEBUG_EDGE
					cout << "   ( " << E2.index + row
						 << " , " << E2.index + col << ") = "
						 << M22[row][col] << endl;
			#endif
			}
	}
	else{
		edgeInteg(edge, mesh, prob, M11);
		
		// M11
		#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << "  M11" << endl;
		#endif
		for(int row = 0; row != M11.size(); ++row)
			for(int col = 0; col != M11[row].size(); ++col){
				this -> addToMA(M11[row][col], E1.index + row, E1.index + col);
			#ifdef __DGSOLVESYS_DEBUG_EDGE
					cout << "   ( " << E1.index + row
						 << " , " << E1.index + col << ") = "
						 << M11[row][col] << endl;
			#endif

			}
	}


	return 0;
}

template <typename MyProblem>
int retrive_localDof_count_element_index(Mesh<MyProblem> &mesh)
{
	int dof(0);
	for(auto itEle = mesh.element.begin(); itEle != mesh.element.end(); ++itEle)
	{
		itEle -> index = dof;
		int tmpLocalDof = 0;
		for(int ver : itEle -> vertex){
			if(mesh.vertex[ver - 1].bctype == 0){
				++dof;
				++tmpLocalDof;
			}
		}
		itEle -> localDof = tmpLocalDof;
	}

	return dof;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::assembleStiff(Mesh<MyProblem> &mesh, MyProblem prob)
{
	#ifdef __DGSOLVESYS_DEBUG
		cout << "start forming system" << endl;
	#endif

	clock_t t = clock();

	this -> dof = retrive_localDof_count_element_index(mesh);
	
	this -> rh = new double [this -> dof];
	memset(this -> rh, 0, (this -> dof) * sizeof(double));
	this -> ma.resize(this -> dof);

	#ifdef __DGSOLVESYS_DEBUG
		cout << " dof = " << this -> dof << endl;
	#endif

	calcDetBE(mesh); //calculate det(B_E) for each element
	
	// assemble element integral related items
	int k = 1;
	for(auto it = mesh.element.begin();
		it != mesh.element.end(); ++it, ++k)
	{

	#ifdef __DGSOLVESYS_DEBUG_LV2
			cout << " assemble element " << k << endl;
	#endif
		assembleElement(*it, mesh, prob);
	}

	//calc penalty
	penaltyOver3 = prob.sigma0 / 3.0;
	penaltyOver6 = prob.sigma0 / 6.0;
	cout << "  penalty : " << penaltyOver3 << "  " << penaltyOver6 << endl;

	// assemble edge integral related items
	k = 1;
	for(auto it = mesh.edge.begin(); it != mesh.edge.end(); ++it, ++k)
	{

	#ifdef __DGSOLVESYS_DEBUG_EDGE
			cout << " assemble edge " << k << endl;
	#endif
		assembleEdge(*it, mesh, prob);
	}

	t = clock() - t;

	#ifdef __DGSOLVESYS_DEBUG
		cout << "finish forming system, t = "
				  << (double) t / CLOCKS_PER_SEC << "s"
				  << endl << endl;
	#endif	
	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::consoleOutput(Mesh<MyProblem> mesh)
{
	std::vector<Element>::iterator it;
	int k;
	for(k = 0, it = mesh.element.begin(); it != mesh.element.end(); it++)
		for(int ver : it -> vertex)
			if(mesh.vertex[ver - 1].bctype == 0)
				cout << mesh.vertex[ver - 1].x << " " << mesh.vertex[ver - 1].y << " " << this -> x[k++] << std::endl;

	return 0;
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::fileOutput(Mesh<MyProblem> mesh, MyProblem prob)
{
	std::ofstream fout((prob.parameters.meshFilename + ".output").c_str());

	std::vector<Element>::iterator it;
	int k;

	for(k = 0, it = mesh.element.begin(); it != mesh.element.end(); it++)
		for(int ver : it -> vertex){
			if(mesh.vertex[ver - 1].bctype == 0)
				fout << mesh.vertex[ver - 1].x << " " << mesh.vertex[ver - 1].y << " " << this -> x[k++] << std::endl;
			else
				fout << mesh.vertex[ver - 1].x << " " << mesh.vertex[ver - 1].y << " "
					 << prob.gd(mesh.vertex[ver - 1].x, mesh.vertex[ver - 1].y) << std::endl;
		}
		

	return 0;		
}

template <typename MyProblem>
int DGSolvingSystem<MyProblem>::triOutput(MyProblem prob, Mesh<MyProblem> mesh)
{
	if(prob.parameters.printResults)
		consoleOutput(mesh);
	if(prob.parameters.fprintResults)
		fileOutput(mesh, prob);
	if(prob.parameters.fprintMA)
		this -> fileOutputMA(mesh, prob);
	if(prob.parameters.fprintRH)
		this -> fileOutputRH(mesh, prob);
	if(prob.parameters.fprintTriplet)
		this -> fileOutputTriplet(mesh, prob);

	return 0;
}

#endif /* TRI_DGSOLVESYS_H */