#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkCallbackCommand.h>
#include <vtkTriangle.h>
#include <vtkPolygon.h>
#include <vtkQuad.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <array>


vtkNew<vtkTextActor> traditionalFpsText;
vtkNew<vtkTextActor> optimizedFpsText;

static void TraditionalFpsCallbackFunction(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData)
{
    vtkRenderer* renderer = static_cast<vtkRenderer*>(caller);
    double timeInSeconds = renderer->GetLastRenderTimeInSeconds();
    double fps = 1.0 / timeInSeconds;
    std::string fpsStr = "Traditional FSP: " + std::to_string(fps);
    traditionalFpsText->SetInput(fpsStr.c_str());
}

static void OptimizedFpsCallbackFunction(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData)
{
    vtkRenderer* renderer = static_cast<vtkRenderer*>(caller);
    double timeInSeconds = renderer->GetLastRenderTimeInSeconds();
    double fps = 1.0 / timeInSeconds;
    std::string fpsStr = "Optimized FSP: " + std::to_string(fps);
    optimizedFpsText->SetInput(fpsStr.c_str());
}


void TraditionalRenderAssembly(vtkSmartPointer<vtkNamedColors> colors)
{
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->SetSize(800, 500);
    renderWindow->SetWindowName("Traditional Rendering");

    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);

    vtkNew<vtkRenderer> renderer;

    // thousands of actors for assemebly
    size_t counts = 1000;
    for (size_t i = 0; i < counts; ++i)
    {
        vtkNew<vtkCylinderSource> cylinder;
        cylinder->SetRadius(3);
        cylinder->SetHeight(120);
        cylinder->SetResolution(24);
        cylinder->SetCenter(i * 8, 0, 0);

        vtkNew<vtkPolyDataMapper> cylinderMapper;
        cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

        vtkNew<vtkActor> cylinderActor;
        cylinderActor->SetMapper(cylinderMapper);
        cylinderActor->GetProperty()->SetColor(
            colors->GetColor4d("Tomato").GetData());
        renderer->AddActor(cylinderActor);
    }

    renderer->SetBackground(colors->GetColor3d("SkyBlue").GetData());

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.5);

    vtkNew<vtkCallbackCommand> callback;
    callback->SetCallback(TraditionalFpsCallbackFunction);
    renderer->AddObserver(vtkCommand::EndEvent, callback);

    renderWindow->AddRenderer(renderer);

    if (traditionalFpsText)
    {
        traditionalFpsText->SetInput("FPS: 0");
        renderer->AddActor(traditionalFpsText);
        vtkTextProperty* txtprop = traditionalFpsText->GetTextProperty();
        txtprop->SetFontFamilyToArial();
        txtprop->BoldOn();
        txtprop->SetFontSize(16);
        traditionalFpsText->SetDisplayPosition(20, 10);
    }
    renderWindow->Render();
    renderWindowInteractor->Start();
}

void OptimizedRenderAssembly(vtkSmartPointer<vtkNamedColors> colors)
{
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->SetSize(800, 500);
    renderWindow->SetWindowName("Optimized Rendering");

    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);

    vtkNew<vtkRenderer> renderer;
    // optimize large assembly by merging to one actor
    vtkNew<vtkPoints> trianglePoints;
    vtkNew<vtkCellArray> triangleCellArray;
    vtkNew<vtkPolyData> trianglesPoly;
    vtkNew<vtkActor> cylinderActor;

    size_t counts = 1000;
    vtkIdType currPointNum = 0;
    for (size_t i = 0; i < counts; ++i)
    {
        vtkNew<vtkCylinderSource> cylinder;
        cylinder->SetRadius(3);
        cylinder->SetHeight(120);
        cylinder->SetResolution(24);
        cylinder->SetCenter(i * 8, 0, 0);
        cylinder->Update();

        vtkPolyData* polyData = cylinder->GetOutput();
        for (vtkIdType j = 0; j < polyData->GetNumberOfPoints(); ++j)
        {
            trianglePoints->InsertNextPoint(polyData->GetPoint(j));
        }
        for (vtkIdType j = 0; j < polyData->GetNumberOfPolys(); ++j)
        {
            auto cell_1 = polyData->GetCell(j);
            if (!cell_1)
                continue;
            int cellType = cell_1->GetCellType();
            switch (cellType)
            {
            case VTK_TRIANGLE:
            {
                vtkNew<vtkTriangle> triangle;
                for (vtkIdType k = 0; k < cell_1->GetNumberOfPoints(); ++k)
                {
                    triangle->GetPointIds()->SetId(k, cell_1->GetPointIds()->GetId(k) + currPointNum);
                }
                triangleCellArray->InsertNextCell(triangle);
            }
            break;
            case VTK_QUAD:
            {
                vtkNew<vtkQuad> quad;
                for (vtkIdType k = 0; k < cell_1->GetNumberOfPoints(); ++k)
                {
                    quad->GetPointIds()->SetId(k, cell_1->GetPointIds()->GetId(k) + currPointNum);
                }
                triangleCellArray->InsertNextCell(quad);
            }
            break;
            case VTK_POLYGON:
            {
                vtkNew<vtkPolygon> poly;
                poly->GetPointIds()->Allocate(cell_1->GetNumberOfPoints());
                for (vtkIdType k = 0; k < cell_1->GetNumberOfPoints(); ++k)
                {
                    poly->GetPointIds()->SetId(k, cell_1->GetPointIds()->GetId(k) + currPointNum);
                }
                triangleCellArray->InsertNextCell(poly);
            }
            break;
            default:
                std::cout << "celltype: " << cellType << std::endl;
                break;
            }

        }
        currPointNum += polyData->GetNumberOfPoints();
    }
    trianglesPoly->SetPoints(trianglePoints);
    trianglesPoly->SetPolys(triangleCellArray);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputData(trianglesPoly);

    cylinderActor->SetMapper(cylinderMapper);
    cylinderActor->GetProperty()->SetColor(
        colors->GetColor4d("Tomato").GetData());
    renderer->AddActor(cylinderActor);

    renderer->SetBackground(colors->GetColor3d("LightGreen").GetData());

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.5);

    vtkNew<vtkCallbackCommand> callback;
    callback->SetCallback(OptimizedFpsCallbackFunction);
    renderer->AddObserver(vtkCommand::EndEvent, callback);

    renderWindow->AddRenderer(renderer);

    if (optimizedFpsText)
    {
        optimizedFpsText->SetInput("FPS: 0");
        renderer->AddActor(optimizedFpsText);
        vtkTextProperty* txtprop = optimizedFpsText->GetTextProperty();
        txtprop->SetFontFamilyToArial();
        txtprop->BoldOn();
        txtprop->SetFontSize(16);
        txtprop->SetColor(colors->GetColor3d("Cornsilk").GetData());
        optimizedFpsText->SetDisplayPosition(20, 10);
    }
    renderWindow->Render();
    renderWindowInteractor->Start();
}

int main(int, char* [])
{
    vtkNew<vtkNamedColors> colors;

    //compare the traditional method
    //TraditionalRenderAssembly(colors);

    //optimized method
    OptimizedRenderAssembly(colors);

    return EXIT_SUCCESS;
}