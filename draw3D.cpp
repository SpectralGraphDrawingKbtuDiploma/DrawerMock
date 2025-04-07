#include <vtkSmartPointer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkCamera.h>
#include <vtkLine.h>
#include <vtkOBJExporter.h>
#include <vtkNew.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRendererCollection.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <string>


class CustomInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
  static CustomInteractorStyle* New();
  vtkTypeMacro(CustomInteractorStyle, vtkInteractorStyleTrackballCamera);

  CustomInteractorStyle() {
    this->CompositeTransform = nullptr;
    this->HUDAxes = nullptr;
    this->Camera = nullptr;
    for (int i = 0; i < 3; i++) {
      this->StartPos[i] = 0.0;
      this->CameraInitialPosition[i] = 0.0;
      this->CameraInitialFocal[i] = 0.0;
      this->CameraInitialViewUp[i] = 0.0;
    }
  }

  void SetCompositeTransform(vtkTransform* transform) {
    this->CompositeTransform = transform;
  }

  void SetStartPos(double pos[3]) {
    for (int i = 0; i < 3; ++i)
      this->StartPos[i] = pos[i];
  }

  void SetHUDAxes(vtkAxesActor* axes) {
    this->HUDAxes = axes;
  }

  void SetCamera(vtkCamera* cam) {
    this->Camera = cam;
  }

  void SetCameraInitial(double pos[3], double focal[3], double viewup[3]) {
    for (int i = 0; i < 3; ++i) {
      this->CameraInitialPosition[i] = pos[i];
      this->CameraInitialFocal[i] = focal[i];
      this->CameraInitialViewUp[i] = viewup[i];
    }
  }

  virtual void OnKeyPress() override {
    vtkRenderWindowInteractor* interactor = this->GetInteractor();
    std::string key = interactor->GetKeySym();
    double angleStep = 5.0;
    bool rotated = false;

    if (key == "Up") {
      this->CompositeTransform->RotateX(angleStep);
      rotated = true;
    }
    else if (key == "Down") {
      this->CompositeTransform->RotateX(-angleStep);
      rotated = true;
    }
    else if (key == "Left") {
      this->CompositeTransform->RotateY(angleStep);
      rotated = true;
    }
    else if (key == "Right") {
      this->CompositeTransform->RotateY(-angleStep);
      rotated = true;
    }
    else if (key == "space") {

      this->CompositeTransform->Identity();
      this->CompositeTransform->Translate(this->StartPos[0], this->StartPos[1], this->StartPos[2]);

      if (this->Camera) {
        this->Camera->SetPosition(this->CameraInitialPosition);
        this->Camera->SetFocalPoint(this->CameraInitialFocal);
        this->Camera->SetViewUp(this->CameraInitialViewUp);
      }
      vtkRenderer* renderer = interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
      if (renderer) {
        renderer->ResetCameraClippingRange();
      }
      rotated = true;
    }
    else {

      vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    if (rotated) {
      this->UpdateHUDAxes();
      interactor->GetRenderWindow()->Render();
    }
  }

protected:
  void UpdateHUDAxes() {
    vtkNew<vtkMatrix4x4> mat;
    this->CompositeTransform->GetMatrix(mat);

    mat->SetElement(0, 3, 0.0);
    mat->SetElement(1, 3, 0.0);
    mat->SetElement(2, 3, 0.0);
    if (this->HUDAxes) {
      this->HUDAxes->SetUserMatrix(mat);
    }
  }

  vtkTransform* CompositeTransform;
  double StartPos[3];
  vtkAxesActor* HUDAxes;
  vtkCamera* Camera;
  double CameraInitialPosition[3];
  double CameraInitialFocal[3];
  double CameraInitialViewUp[3];
};

vtkStandardNewMacro(CustomInteractorStyle);


std::vector<std::array<double, 3>> read_vertices(const std::string& filename) {
    std::vector<std::array<double, 3>> vertices;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Could not open vertex file: " << filename << std::endl;
        return vertices;
    }
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::vector<double> vals;
        double v;
        while (iss >> v) {
            vals.push_back(v);
        }
        if (vals.size() == 2) {
            vertices.push_back({vals[0], vals[1], 0.0});
        }
        else if (vals.size() >= 3) {
            vertices.push_back({vals[0], vals[1], vals[2]});
        }
    }
    return vertices;
}


std::vector<std::pair<int, int>> read_edges(const std::string& filename) {
    std::vector<std::pair<int, int>> edges;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Could not open edge file: " << filename << std::endl;
        return edges;
    }
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        int u, v;
        if (iss >> u >> v) {
            edges.push_back(std::make_pair(u, v));
        }
    }
    return edges;
}

int main(int argc, char* argv[]) {

    std::string vertex_file = "./tmp/graph/embedding.txt";
    std::string edge_file   = "graph.txt";


    auto vertices = read_vertices(vertex_file);
    auto edges = read_edges(edge_file);

    std::cout << "Loaded " << vertices.size() << " vertices" << std::endl;
    std::cout << "Loaded " << edges.size() << " edges" << std::endl;

    if (vertices.empty()) {
        std::cerr << "No vertices loaded!" << std::endl;
        return EXIT_FAILURE;
    }


    vtkNew<vtkPoints> vtk_points;
    for (const auto& pt : vertices) {
        vtk_points->InsertNextPoint(pt[0], pt[1], pt[2]);
    }


    vtkNew<vtkCellArray> vtk_lines;
    for (const auto& edge : edges) {
        int u = edge.first;
        int v = edge.second;
        if (u < 0 || v < 0 || u >= static_cast<int>(vertices.size()) || v >= static_cast<int>(vertices.size()))
            continue;
        vtkNew<vtkLine> line;
        line->GetPointIds()->SetId(0, u);
        line->GetPointIds()->SetId(1, v);
        vtk_lines->InsertNextCell(line);
    }


    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(vtk_points);
    polyData->SetLines(vtk_lines);


    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(polyData);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(65.0/255.0, 105.0/255.0, 225.0/255.0); // royal blue
    actor->GetProperty()->SetLineWidth(2);

    vtkNew<vtkTransform> compositeTransform;
    double startPos[3] = {0.0, 0.0, 0.0};
    compositeTransform->Translate(startPos);
    actor->SetUserTransform(compositeTransform);


    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.95, 0.95, 0.95);

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);


    renderer->AddActor(actor);


    vtkNew<vtkAxesActor> hudAxes;


    renderer->ResetCamera();
    renderWindow->Render();


    vtkCamera* camera = renderer->GetActiveCamera();
    double cameraInitialPosition[3];
    double cameraInitialFocal[3];
    double cameraInitialViewUp[3];
    camera->GetPosition(cameraInitialPosition);
    camera->GetFocalPoint(cameraInitialFocal);
    camera->GetViewUp(cameraInitialViewUp);


    vtkNew<CustomInteractorStyle> style;
    style->SetCompositeTransform(compositeTransform);
    style->SetStartPos(startPos);
    style->SetHUDAxes(hudAxes);
    style->SetCamera(camera);
    style->SetCameraInitial(cameraInitialPosition, cameraInitialFocal, cameraInitialViewUp);
    interactor->SetInteractorStyle(style);


    vtkNew<vtkOrientationMarkerWidget> orientationWidget;
    orientationWidget->SetOrientationMarker(hudAxes);
    orientationWidget->SetInteractor(interactor);
    orientationWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    orientationWidget->EnabledOn();
    orientationWidget->InteractiveOff();

    renderWindow->SetSize(800, 600);
    renderWindow->SetWindowName("3D Graph with Rotating Coordinates (VTK - C++)");


    vtkNew<vtkOBJExporter> objExporter;
    objExporter->SetFilePrefix("graph"); // This will create "graph.obj" and "graph.mtl"
    objExporter->SetRenderWindow(renderWindow);
    objExporter->Write();

    interactor->Start();

    return EXIT_SUCCESS;
}
