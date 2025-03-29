import vtk

class CustomInteractorStyle(vtk.vtkInteractorStyleTrackballCamera):
    def __init__(self, compositeTransform, startPos, hudAxes, camera, camera_initial):
        super(CustomInteractorStyle, self).__init__()
        self.compositeTransform = compositeTransform
        self.startPos = startPos
        self.hudAxes = hudAxes
        self.camera = camera
        self.camera_initial = camera_initial
        # Add an observer for key press events.
        self.AddObserver("KeyPressEvent", self.on_key_press)

    def updateHUDAxes(self):
        mat = vtk.vtkMatrix4x4()
        self.compositeTransform.GetMatrix(mat)
        # Zero out translation components (we want only rotation in the HUD).
        mat.SetElement(0, 3, 0.0)
        mat.SetElement(1, 3, 0.0)
        mat.SetElement(2, 3, 0.0)
        self.hudAxes.SetUserMatrix(mat)

    def on_key_press(self, obj, event):
        key = self.GetInteractor().GetKeySym()
        angleStep = 5.0  # degrees per key press
        rotated = False

        if key == "Up":
            self.compositeTransform.RotateX(angleStep)
            # print("Rotated composite around X by", angleStep)
            rotated = True
        elif key == "Down":
            self.compositeTransform.RotateX(-angleStep)
            # print("Rotated composite around X by", -angleStep)
            rotated = True
        elif key == "Left":
            self.compositeTransform.RotateY(angleStep)
            # print("Rotated composite around Y by", angleStep)
            rotated = True
        elif key == "Right":
            self.compositeTransform.RotateY(-angleStep)
            # print("Rotated composite around Y by", -angleStep)
            rotated = True
        elif key == "space":
            # Reset composite transform.
            self.compositeTransform.Identity()
            self.compositeTransform.Translate(*self.startPos)
            # Also restore the camera's initial parameters.
            self.camera.SetPosition(*self.camera_initial['position'])
            self.camera.SetFocalPoint(*self.camera_initial['focal_point'])
            self.camera.SetViewUp(*self.camera_initial['view_up'])
            # Reset the camera clipping range.
            renderer = self.GetInteractor().GetRenderWindow().GetRenderers().GetFirstRenderer()
            renderer.ResetCameraClippingRange()
            # print("Reset composite transform and camera to initial state.")
            rotated = True
        else:
            super(CustomInteractorStyle, self).OnKeyPress()

        if rotated:
            self.updateHUDAxes()
            self.GetInteractor().GetRenderWindow().Render()

def read_vertices(filename):
    vertices = []
    with open(filename, "r") as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            if len(parts) == 2:
                x, y = map(float, parts)
                z = 0.0
            else:
                x, y, z = map(float, parts[:3])
            vertices.append((x, y, z))
    return vertices

def read_edges(filename):
    edges = []
    with open(filename, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            u, v = map(int, parts[:2])
            edges.append((u, v))
    return edges

def main():
    # File paths (adjust as needed)
    vertex_file = "./tmp/graph/embedding.txt"
    edge_file   = "graph.txt"

    # Read data.
    vertices = read_vertices(vertex_file)
    edges = read_edges(edge_file)

    print("Loaded", len(vertices), "vertices")
    print("Loaded", len(edges), "edges")

    if not vertices:
        print("No vertices loaded!")
        return

    # Create vtkPoints and add vertices.
    vtk_points = vtk.vtkPoints()
    for pt in vertices:
        vtk_points.InsertNextPoint(pt)

    # Create a vtkCellArray for the edges.
    vtk_lines = vtk.vtkCellArray()
    for (u, v) in edges:
        if u < 0 or v < 0 or u >= len(vertices) or v >= len(vertices):
            continue
        line = vtk.vtkLine()
        line.GetPointIds().SetId(0, u)
        line.GetPointIds().SetId(1, v)
        vtk_lines.InsertNextCell(line)

    # Create polydata to hold the graph.
    polyData = vtk.vtkPolyData()
    polyData.SetPoints(vtk_points)
    polyData.SetLines(vtk_lines)

    # Create a mapper and actor for the graph.
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputData(polyData)

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(65/255.0, 105/255.0, 225/255.0)  # royal blue
    actor.GetProperty().SetLineWidth(2)

    # Create a composite transform and apply it only to the graph actor.
    compositeTransform = vtk.vtkTransform()
    startPos = (0.0, 0.0, 0.0)
    compositeTransform.Translate(*startPos)
    actor.SetUserTransform(compositeTransform)

    # Create renderer, render window, and interactor.
    renderer = vtk.vtkRenderer()
    # Set background to a white-grey color.
    renderer.SetBackground(0.95, 0.95, 0.95)

    renderWindow = vtk.vtkRenderWindow()
    renderWindow.AddRenderer(renderer)

    interactor = vtk.vtkRenderWindowInteractor()
    interactor.SetRenderWindow(renderWindow)

    # Add the graph actor.
    renderer.AddActor(actor)

    # Create a separate axes actor for the orientation marker (HUD).
    hudAxes = vtk.vtkAxesActor()

    # Reset the camera to include all actors.
    renderer.ResetCamera()
    renderWindow.Render()

    # Get the active camera and store its initial parameters.
    camera = renderer.GetActiveCamera()
    camera_initial = {
        'position': camera.GetPosition(),
        'focal_point': camera.GetFocalPoint(),
        'view_up': camera.GetViewUp()
    }

    # Instantiate the custom interactor style with the composite transform, hudAxes, and camera.
    style = CustomInteractorStyle(compositeTransform, startPos, hudAxes, camera, camera_initial)
    interactor.SetInteractorStyle(style)

    # Add an orientation marker widget to display the HUD axes.
    orientationWidget = vtk.vtkOrientationMarkerWidget()
    orientationWidget.SetOrientationMarker(hudAxes)
    orientationWidget.SetInteractor(interactor)
    orientationWidget.SetViewport(0.0, 0.0, 0.2, 0.2)
    orientationWidget.EnabledOn()
    orientationWidget.InteractiveOff()

    renderWindow.SetSize(800, 600)
    renderWindow.SetWindowName("3D Graph with Rotating Coordinates (VTK - Python)")

    objExporter = vtk.vtkOBJExporter()
    objExporter.SetFilePrefix("graph")  # This will create "graph.obj" and "graph.mtl"
    objExporter.SetRenderWindow(renderWindow)
    objExporter.Write()

    interactor.Start()

if __name__ == "__main__":
    main()
