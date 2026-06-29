class Demo {
private:
    int secret;
    int code;
public:
    int value;
    void setSecret(int s) {
        secret = s;
    }
    int getSecret() {
        return secret;
    }
protected:
    int internal;
};

int main() {
    Demo d;
    d.value = 42;
    d.setSecret(99);
    int result;
    result = d.getSecret();
    return result;
}
