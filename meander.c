#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

//grid value constants
#define BLANK 0
#define START 1
#define END 2
#define PATH 3

typedef struct point {
    int x;
    int y;
} point;

typedef struct grid {
    int height;
    int width;
    int** values;
    point start;
    point end;
} grid;

grid *createGrid(int height, int width, point start, point end) {
    grid *g = malloc(sizeof(grid));
    g->height = height;
    g->width = width;
    g->start = start;
    g->end = end;
    g->values = malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++){
        g->values[i] = malloc(width * sizeof(int));
        for (int j = 0; j < width; j++) {
            g->values[i][j] = BLANK;
        }
    }
    g->values[g->start.y][g->start.x] = START;
    g->values[g->end.y][g->end.x] = END;
    return g;
}

void releaseGrid(grid *g) {
    if (g == NULL) return;
    for (int i = 0; i < g->height; i++) {
        free(g->values[i]);
    }
    free(g->values);
    free(g);
}

grid *copyGrid(grid *toCopy) {
    grid *g = malloc(sizeof(grid));
    g->height = toCopy->height;
    g->width = toCopy->width;
    g->values = malloc(toCopy->height * sizeof(int *));
    for (int i = 0; i < toCopy->height; i++) {
        g->values[i] = malloc(toCopy->width * sizeof(int));
        for (int j = 0; j < toCopy->width; j++) {
            g->values[i][j] = toCopy->values[i][j];
        }
    }
    g->start = toCopy->start;
    g->end = toCopy->end;
    return g;
}

typedef struct gridNode {
    grid *entry;
    struct gridNode *next; 
    point pathTerminus;
} gridNode;

gridNode *createGridNode(grid *entry, point pathTerminus){
    gridNode *gn = malloc(sizeof(gridNode));
    gn->entry = entry;
    gn->next = NULL;
    gn->pathTerminus = pathTerminus;
    return gn;
}

void releaseGridNode(gridNode *g) {
    free(g);
}

gridNode *copyGridNode(gridNode *toCopy) {
    gridNode* gn = malloc(sizeof(gridNode));
    gn->entry = copyGrid(toCopy->entry);
    gn->next = toCopy->next;
    gn->pathTerminus = toCopy->pathTerminus;
    return gn;
}

typedef struct gridNodeStack {
   gridNode *top; 
} gridNodeStack;

gridNodeStack *createGridNodeStack() {
    gridNodeStack *gns = malloc(sizeof(gridNodeStack));
    gns->top = NULL;
    return gns;
}

void releaseGridNodeStack(gridNodeStack *gns) {
    free(gns);
}

void gridNodeStack_push(gridNodeStack *gns, gridNode *gn) {
    gn->next = gns->top;
    gns->top = gn;
}

gridNode *gridNodeStack_pop(gridNodeStack *gns) {
    gridNode *gn = gns->top;
    if (gn) gns->top = gn->next;
    return gn;
}

typedef struct yConstraints {
    int minY;
    int maxY;
} yConstraints;

yConstraints calculateYConstraints(grid *toWalk, int x){
    int pathEndColumn = toWalk->end.x-1;
    int diffX = pathEndColumn - x;
    yConstraints constraints;
    constraints.maxY = toWalk->end.y + diffX;
    constraints.minY = toWalk->end.y - diffX;
    return constraints;
} 

typedef struct moveConstraints {
    bool allowDown;
    bool allowFlat;
    bool allowUp;
} moveConstraints;

moveConstraints calculateMoveConstraints(grid* toWalk, int x, int y) {
    yConstraints allowedY = calculateYConstraints(toWalk, x);
    moveConstraints allowedMoves;

    allowedMoves.allowDown = y > 0 && y > allowedY.minY;
    allowedMoves.allowFlat = y >= allowedY.minY && y <= allowedY.maxY;
    allowedMoves.allowUp = y < toWalk->height-1 && y < allowedY.maxY;

    return allowedMoves;
}

int getDirectionCount(moveConstraints allowedMoves) {
    int directionCount = 0;
    if (allowedMoves.allowDown) directionCount++;
    if (allowedMoves.allowFlat) directionCount++;
    if (allowedMoves.allowUp) directionCount++;
    return directionCount;
}

int getDirectionSelect(moveConstraints allowedMoves) {
    return rand() % getDirectionCount(allowedMoves);
}

int calculateYMotion(moveConstraints allowedMoves) {
    switch (getDirectionSelect(allowedMoves)) {
        case 0:
            if (allowedMoves.allowDown) {
                return -1;
            } else if (allowedMoves.allowFlat) {
                return 0;
            } else if (allowedMoves.allowUp) {
                return 1;
            }
        case 1:
            if (allowedMoves.allowFlat) {
                return 0;
            } else if (allowedMoves.allowUp){
                return 1;
                break;
            }
        default:
            return 1;
    }
}

void meander(grid *toWalk){
    srand(time(NULL));
    int x = toWalk->start.x;
    int y = toWalk->start.y;
    for (bool first = true; x < toWalk->end.x; x++, first = false) {
        if (!first) toWalk->values[y][x] = PATH;
        moveConstraints allowedMoves = calculateMoveConstraints(toWalk, x, y);
        y += calculateYMotion(allowedMoves);
    }
}

gridNode *takeStep(gridNode *steppingPoint, int yMotion) {
    gridNode *nextStep = copyGridNode(steppingPoint);
    nextStep->pathTerminus.x++;
    nextStep->pathTerminus.y += yMotion;
    nextStep->entry->values
        [nextStep->pathTerminus.y]
        [nextStep->pathTerminus.x] = PATH;
    return nextStep;
}

void takeNextSteps(gridNode *steppingPoint, gridNodeStack *toSolve) {
    moveConstraints allowedMoves = calculateMoveConstraints(
                                        steppingPoint->entry, 
                                        steppingPoint->pathTerminus.x,
                                        steppingPoint->pathTerminus.y);
    if (allowedMoves.allowDown) {
        gridNodeStack_push(toSolve, takeStep(steppingPoint, -1));
    }
    if (allowedMoves.allowFlat) {
        gridNodeStack_push(toSolve, takeStep(steppingPoint, 0));
    }
    if (allowedMoves.allowUp) {
        gridNodeStack_push(toSolve, takeStep(steppingPoint, 1));
    }
}

void allMeanders(gridNodeStack *solutions, grid *toWalk){
    gridNodeStack *toSolve = createGridNodeStack();
    //copy toWalk so the original is not freed in this method as part
    //of the solving loop
    gridNode *currentNode = createGridNode(copyGrid(toWalk), toWalk->start);
    int endX = toWalk->end.x-1;
    while (currentNode) {
        if (currentNode->pathTerminus.x == endX) {
            gridNodeStack_push(solutions, currentNode);
        } else {
            takeNextSteps(currentNode, toSolve);
            releaseGrid(currentNode->entry);
            releaseGridNode(currentNode);
        }
        currentNode = gridNodeStack_pop(toSolve);
    }
}

void paintCellHorizontalBorder(int rowLength) {
    for (int i = 0; i < rowLength; i++) {
        printf("+-");
    }
    printf("+\n");
}

void paintCells(int *row, int rowLength){
    for (int i = 0; i < rowLength; i++){
        printf("|");
        switch (row[i]) {
            case BLANK: 
                printf(" ");
                break;
            case START:
                printf("A");
                break;
            case END:
                printf("B");
                break;
            case PATH:
                printf("x");
                break;
        }
    }
    printf("|\n");
}

void displayGrid(grid *toDisplay) {
    for (int i = 0; i < toDisplay->height; i++){
        paintCellHorizontalBorder(toDisplay->width);
        paintCells(toDisplay->values[i], toDisplay->width);
    }
    paintCellHorizontalBorder(toDisplay->width);
}

grid *createInitialGrid(int height, int width, point start, point end) {
    return createGrid(height, width, start, end);
}

void doMeander(grid* unsolvedGrid) {
    meander(unsolvedGrid);
    displayGrid(unsolvedGrid);
}

void doAllMeanders(grid* unsolvedGrid) {
    gridNodeStack *solutions = createGridNodeStack();
    allMeanders(solutions, unsolvedGrid);
    gridNode *solution = gridNodeStack_pop(solutions);
    for (int i = 0; solution; i++) {
        printf("Solution %d:\n", i);
        displayGrid(solution->entry);
        releaseGrid(solution->entry);
        releaseGridNode(solution);
        solution = gridNodeStack_pop(solutions);
    }
    releaseGridNodeStack(solutions);
}

grid *runGridCreateMenu() {
    int height, width;
    point start, end;
    printf("Please enter grid height,width,startX,startY,endX,endY: ");
    scanf("%d,%d,%d,%d,%d,%d", 
            &height, &width, &start.x, &start.y, &end.x, &end.y);
    return createInitialGrid(height, width, start, end);
}

void runMeanderMenu(){
    while(1) {
        grid* unsolvedGrid = runGridCreateMenu();
        printf("Do you want:\n");
        printf("1) A random walk\n");
        printf("2) All valid paths\n");
        printf("3) Quit\n");
        printf("Please enter number of choice:");
        int choice;
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                doMeander(unsolvedGrid);
                break;
            case 2:
                doAllMeanders(unsolvedGrid);
                break;
            case 3:
                printf("\nGoodbye.\n");
                return;
            default:
                "Did not understand choice. Try again";
        }
        releaseGrid(unsolvedGrid);
    }
}

void main(int argc, char **argv) {
    runMeanderMenu();
}
