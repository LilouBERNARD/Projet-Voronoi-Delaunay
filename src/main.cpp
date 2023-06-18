#include "application_ui.h"
#include "SDL2_gfxPrimitives.h"
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <algorithm>
#include <random>

#define EPSILON 0.0001f

struct Coords
{ 
    int x, y;

    bool operator==(const Coords& other) const
    {
        return x == other.x && y == other.y;
    }
};

struct Segment
{
    Coords p1, p2;

    bool operator==(const Segment& other) const
    {
        return p1 == other.p1 && p2 == other.p2;
    }
};

struct Triangle
{
    Coords p1, p2, p3;
    bool complet=false;
};

struct Polygon {
    std::vector<int> xCoords;
    std::vector<int> yCoords;
};

struct Application
{
    int width, height;
    Coords focus{100, 100};

    std::vector<Coords> points;
    std::vector<Triangle> triangles;
    std::vector<Polygon> polygons;
};



bool compareCoords(Coords point1, Coords point2)
{
    if (point1.y == point2.y)
        return point1.x < point2.x;
    return point1.y < point2.y;
}

void drawSegments(SDL_Renderer *renderer, const std::vector<Segment> &segments)
{
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        lineRGBA(
            renderer,
            segments[i].p1.x, segments[i].p1.y,
            segments[i].p2.x, segments[i].p2.y,
            240, 240, 20, SDL_ALPHA_OPAQUE);
    }
}


void drawPoints(SDL_Renderer *renderer, const std::vector<Coords> &points)
{
    for (std::size_t i = 0; i < points.size(); i++)
    {
        filledCircleRGBA(renderer, points[i].x, points[i].y, 3, 240, 240, 23, SDL_ALPHA_OPAQUE);
    }
}


void drawTriangles(SDL_Renderer *renderer, const std::vector<Triangle> &triangles)
{
    for (std::size_t i = 0; i < triangles.size(); i++)
    {
        const Triangle& t = triangles[i];
        trigonRGBA(
            renderer,
            t.p1.x, t.p1.y,
            t.p2.x, t.p2.y,
            t.p3.x, t.p3.y,
            0, 240, 160, SDL_ALPHA_OPAQUE
        );
    }
}



/*
   Détermine si un point se trouve dans un cercle définit par trois points
   Retourne, par les paramètres, le centre et le rayon
*/
bool CircumCircle(
    float pX, float pY,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float *xc, float *yc, float *rsqr
)
{
    float m1, m2, mx1, mx2, my1, my2;
    float dx, dy, drsqr;
    float fabsy1y2 = fabs(y1 - y2);
    float fabsy2y3 = fabs(y2 - y3);

    /* Check for coincident points */
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return (false);

    if (fabsy1y2 < EPSILON)
    {
        m2 = -(x3 - x2) / (y3 - y2);
        mx2 = (x2 + x3) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (x2 + x1) / 2.0;
        *yc = m2 * (*xc - mx2) + my2;
    }
    else if (fabsy2y3 < EPSILON)
    {
        m1 = -(x2 - x1) / (y2 - y1);
        mx1 = (x1 + x2) / 2.0;
        my1 = (y1 + y2) / 2.0;
        *xc = (x3 + x2) / 2.0;
        *yc = m1 * (*xc - mx1) + my1;
    }
    else
    {
        m1 = -(x2 - x1) / (y2 - y1);
        m2 = -(x3 - x2) / (y3 - y2);
        mx1 = (x1 + x2) / 2.0;
        mx2 = (x2 + x3) / 2.0;
        my1 = (y1 + y2) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
        {
            *yc = m1 * (*xc - mx1) + my1;
        }
        else
        {
            *yc = m2 * (*xc - mx2) + my2;
        }
    }

    dx = x2 - *xc;
    dy = y2 - *yc;
    *rsqr = dx * dx + dy * dy;

    dx = pX - *xc;
    dy = pY - *yc;
    drsqr = dx * dx + dy * dy;

    return ((drsqr - *rsqr) <= EPSILON ? true : false);
}

void delaunayTriangulation(Application &app)
{
    // Trier les points selon x
    std::sort(app.points.begin(), app.points.end(), compareCoords);

    
    app.triangles.clear();

    Triangle grandTriangle{
        Coords{-1000, -1000},
        Coords{500, 3000},
        Coords{1500, -1000}
    };

    app.triangles.push_back(grandTriangle);

    // Parcourir chaque point du repère
    for (const Coords& point : app.points)
    {
        std::list<Segment> segments;

        // Parcourir chaque triangle déjà créé
        for (auto it = app.triangles.begin(); it != app.triangles.end();)
        {
            const Triangle& triangle = *it;

            // Vérifier si le cercle circonscrit contient le point
            float xc, yc, rsqr;
            if (CircumCircle(
                point.x, point.y,
                triangle.p1.x, triangle.p1.y,
                triangle.p2.x, triangle.p2.y,
                triangle.p3.x, triangle.p3.y,
                &xc, &yc, &rsqr
            ))
            {
                
                segments.push_back(Segment{ triangle.p1, triangle.p2 });
                segments.push_back(Segment{ triangle.p2, triangle.p3 });
                segments.push_back(Segment{ triangle.p3, triangle.p1 });

                
                it = app.triangles.erase(it);
            }
            else
            {
                ++it;
            }
        }

        segments.unique();

        // Pour chaque segment dans la liste, créer un nouveau triangle
        for (const Segment& segment : segments)
        {
            Triangle newTriangle{
                segment.p1,
                segment.p2,
                point,
                true 
            };
            app.triangles.push_back(newTriangle);
        }
    }

    // Supprimer les triangles qui contiennent un sommet du grand triangle
    app.triangles.erase(std::remove_if(app.triangles.begin(), app.triangles.end(),
        [](const Triangle& triangle) {
            return triangle.p1 == Coords{-1000, -1000} ||
                triangle.p1 == Coords{500, 3000} ||
                triangle.p1 == Coords{1500, -1000} ||
                triangle.p2 == Coords{-1000, -1000} ||
                triangle.p2 == Coords{500, 3000} ||
                triangle.p2 == Coords{1500, -1000} ||
                triangle.p3 == Coords{-1000, -1000} ||
                triangle.p3 == Coords{500, 3000} ||
                triangle.p3 == Coords{1500, -1000};
        }), app.triangles.end());

}



void drawPolygons(SDL_Renderer* renderer, const std::vector<Polygon>& polygons) 
{

    for (const Polygon& polygon : polygons) 
    {
        int r, g, b; 
        r = rand() % 256; // Générer la couleur aléatoire
        g = rand() % 256;
        b = rand() % 256;


        SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
        const size_t numPoints = polygon.xCoords.size();

        Sint16* vx = new Sint16[numPoints];
        Sint16* vy = new Sint16[numPoints];

        for (size_t i = 0; i < numPoints; ++i) 
        {
            vx[i] = polygon.xCoords[i];
            vy[i] = polygon.yCoords[i];
        }

        filledPolygonRGBA(renderer, vx, vy, numPoints, r, g, b, SDL_ALPHA_OPAQUE);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        polygonRGBA(renderer, vx, vy, numPoints, 0, 0, 0, SDL_ALPHA_OPAQUE);

        delete[] vx;
        delete[] vy;
    }
 
}
  


void constructVoronoi(Application &app)
{
    // Effectuer la triangulation de Delaunay
    delaunayTriangulation(app);

    // Initialiser les polygones de Voronoi
    app.polygons.clear();

    // Parcourir chaque point du repère
    for (const Coords& point : app.points)
    {
        Polygon polygon;

        // Parcourir chaque triangle déjà créé
        for (const Triangle& triangle : app.triangles)
        {
            if (triangle.p1 == point || triangle.p2 == point || triangle.p3 == point)
            {
                // Calcule des coordonnées du cercle circonscrit au triangle
                float xc, yc, rsqr;
                CircumCircle(
                    point.x, point.y,
                    triangle.p1.x, triangle.p1.y,
                    triangle.p2.x, triangle.p2.y,
                    triangle.p3.x, triangle.p3.y,
                    &xc, &yc, &rsqr
                );

                // Ajouter les coordonnées du centre du cercle au polygone
                polygon.xCoords.push_back(static_cast<int>(xc));
                polygon.yCoords.push_back(static_cast<int>(yc));
            }
        }

        // Trier les points du polygone selon x
        std::sort(polygon.xCoords.begin(), polygon.xCoords.end());

        // Ajouter le polygone à la liste des polygones de Voronoi
        app.polygons.push_back(polygon);
    }
    
}

void draw(SDL_Renderer *renderer, Application &app)
{
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    drawPolygons(renderer, app.polygons);
    drawPoints(renderer, app.points);

    /*Elever drawTriangles() du commentaire pour voir la triangulation de Delaunay */
    //drawTriangles(renderer, app.triangles);

    SDL_Rect focusRect = { app.focus.x - 5, app.focus.y - 5, 10, 10 };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(renderer, &focusRect);
}


bool handleEvent(Application &app)
{
    /* Remplissez cette fonction pour gérer les inputs utilisateurs */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        else if (e.type == SDL_WINDOWEVENT_RESIZED)
        {
            app.width = e.window.data1;
            app.height = e.window.data1;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
        }
        else if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                app.focus.x = e.button.x;
                app.focus.y = e.button.y;
                app.points.clear();
            }
            else if (e.button.button == SDL_BUTTON_LEFT)
            {
                app.focus.y = 0;
                app.points.push_back(Coords{e.button.x, e.button.y});
                constructVoronoi(app);
            
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    SDL_Window *gWindow;
    SDL_Renderer *renderer;
    Application app{720, 720, Coords{0, 0}};
    bool is_running = true;

    // Creation de la fenetre
    gWindow = init("Awesome Voronoi", 720, 720);

    if (!gWindow)
    {
        SDL_Log("Failed to initialize!\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_PRESENTVSYNC

    /*  GAME LOOP  */
    while (true)
    {
        // INPUTS
        is_running = handleEvent(app);
        if (!is_running)
            break;

        // EFFACAGE FRAME
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // DESSIN FRAME
        draw(renderer, app);

        // VALIDATION FRAME
        SDL_RenderPresent(renderer);

        // PAUSE en ms
        SDL_Delay(1000 / 30);
    }

    // Free resources and close SDL
    close(gWindow, renderer);

    return 0;
}
