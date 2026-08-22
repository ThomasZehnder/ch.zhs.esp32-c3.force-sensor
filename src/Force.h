#ifndef FORCE_H
#define FORCE_H

class clForce
{
public:
    void setup();
    void loop();

    void tare();
    void calibrate(float knownForceNewton);

private:
    void clearHistory();
};

extern clForce Force;

#endif // FORCE_H
