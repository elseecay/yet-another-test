#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>

using namespace std;


// Напишите функцию, которая принимает на вход знаковое целое число и печатает его двоичное представление
// Не используя библиотечных классов или функций

void print_byte(uint8_t value)
{
    for (int8_t i = 7; i >= 0; --i)
        cout << char(((value >> i) & 1) + '0');
}

template<typename T>
void print_repr(T value)
{
    static_assert (std::is_signed_v<T>);

    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    for (int i = 0; i < sizeof(T); ++i)
    {
        print_byte(*(ptr + i));
        if (i < sizeof(T) - 1)
            cout << ' ';
    }
    cout << endl;
}

template<typename T>
void t1(T value)
{
    using lld = long long signed int;
    using u = unsigned int;

    printf("Repr of %lld (%u bits) is: ", lld(value), u(sizeof(T) * 8));
    print_repr(value);
}


// Напишите функцию, удаляющую последовательно дублирующиеся символы в строке

void remove_dups(char* str)
{
    if (!str[0])
        return;
    char cur = str[0];
    char* last_chr_ptr = str;
    for (uint64_t i = 1;; ++i)
    {
        if (!str[i])
            break;
        if (str[i] == cur)
            continue;
        cur = str[i];
        *(++last_chr_ptr) = cur;
    }
    *(last_chr_ptr + 1) = '\0';
}

void t2(const char* str)
{
    std::string s(str);
    remove_dups(s.data());
    cout << "\'" << str << "\'" << " no dups is \'" << s.data() << "\'" << endl;
}


// Реализуйте функции сериализации и десериализации двусвязного списка в бинарном формате в файл,
// алгоритмическая сложность решения должна быть меньше квадратичной
// структуру list_node модифицировать нельзя

struct list_node
{
    list_node* prev;
    list_node* next;
    list_node* rand;
    std::string data;
};

class list
{
public:
    list() : m_head(nullptr), m_tail(nullptr), m_count(0) { }

    list(const list& other)
    {
        allocn(other.m_count);

        auto src_to_index = other.to_index_map();
        auto dest_to_ptr = to_ptr_map();

        for (list_node* src = other.m_head, *dest = m_head; src; src = src->next, dest = dest->next)
        {
            dest->data = src->data;
            dest->rand = dest_to_ptr[src_to_index[src->rand]];
        }
    }

    ~list()
    {
        clear();
    }

    uint64_t size() const noexcept
    {
        return m_count;
    }

    void push_back(std::string data, uint64_t rand_index)
    {
        auto node = new list_node{nullptr, nullptr, nullptr, std::move(data)};
        
        if (!m_count)
        {
            m_head = node;
            m_tail = m_head;
        }
        else
        {
            node->prev = m_tail;
            m_tail->next = node;
            m_tail = node;
        }
        node->rand = get_node_ptr(rand_index);
        ++m_count;
    }

    bool operator==(const list& other) const
    {
        auto a_to_index = to_index_map();
        auto b_to_index = other.to_index_map();

        list_node *a, *b;
        for (a = m_head, b = other.m_head; a && b; a = a->next, b = b->next)
        {
            if (a->data != b->data || a_to_index[a->rand] != b_to_index[b->rand])
                return false;
        }
        return !a && !b;
    }

    void serialize(FILE* f) const
    {
        unordered_map<list_node*, uint64_t> to_index = to_index_map();
        fwrite(&m_count, sizeof(uint64_t), 1, f);
        for (list_node* cur = m_head; cur; cur = cur->next)
        {
            fwrite(cur->data.data(), (cur->data.size() + 1) * sizeof(char), 1, f);
            fwrite(&to_index[cur->rand], sizeof(uint64_t), 1, f);
        }
    }

    void deserialize(FILE* f)
    {
        clear();

        uint64_t count;
        fread(&count, sizeof(uint64_t), 1, f);
        if (!count)
            return;

        // already allocated nodes can be used there
        allocn(count);

        unordered_map<uint64_t, list_node*> to_ptr = to_ptr_map();
        list_node* cur = m_head;
        for (uint64_t i = 0; i < count; ++i, cur = cur->next)
        {
            std::string data;
            uint64_t rand;

            while (true)
            {
                char c;
                fread(&c, sizeof(char), 1, f);
                if (!c)
                    break;
                data.push_back(c);
            }

            fread(&rand, sizeof(uint64_t), 1, f);

            cur->data = std::move(data);
            cur->rand = to_ptr[rand];
        }
    }

    void clear()
    {
        while (m_head)
        {
            list_node* nxt = m_head->next;
            delete m_head;
            m_head = nxt;
        }
        m_tail = nullptr;
        m_count = 0;
    }

private:
    void allocn(uint64_t count)
    {
        m_count = count;
        if (!count)
        {
            m_head = nullptr;
            m_tail = nullptr;
            return;
        }
        m_head = new list_node{nullptr, nullptr, nullptr, ""};
        list_node* cur = m_head;
        for (uint64_t i = 1; i < count; ++i, cur = cur->next)
            cur->next = new list_node{cur, nullptr, nullptr, ""};
        m_tail = cur;
    }

    unordered_map<list_node*, uint64_t> to_index_map() const
    {
        unordered_map<list_node*, uint64_t> result;
        for (list_node* cur = m_head; cur; cur = cur->next)
            result[cur] = result.size();
        result[nullptr] = result.size();
        return result;
    }

    unordered_map<uint64_t, list_node*> to_ptr_map() const
    {
        unordered_map<uint64_t, list_node*> result;
        list_node* cur = m_head;
        for (uint64_t i = 0; i < m_count; ++i, cur = cur->next)
            result[i] = cur;
        result[m_count] = nullptr;
        return result;
    }

    list_node* get_node_ptr(uint64_t index) const
    {
        if (index >= m_count)
            return nullptr;
        list_node* cur = m_head;
        for (uint64_t i = 1; i <= index; ++i)
            cur = cur->next;
        return cur;
    }

private:
    list_node* m_head;
    list_node* m_tail;
    uint64_t m_count;
};

void t3()
{
    auto rand_str = [](int size)
    {
        std::string result;
        for (int i = 0; i < size; ++i)
        {
            result.push_back('0' + rand() % 10);
        }
        return result;
    };

    for (int i = 0; i < 100; ++i)
    {
        list l1, l2;
        for (int j = 0; j < i; ++j)
            l1.push_back(rand_str(j), rand() % (j + 1));
        list l3(l1);     

        auto f = fopen("list.bin", "wb");
        l1.serialize(f);
        fclose(f);

        f = fopen("list.bin", "rb");
        l2.deserialize(f);
        fclose(f);

        assert(l2 == l3);
    }

    cout << "PASSED" << endl;
}


// ***********************************

int main(int argc, const char* argv[])
{
    t1(int8_t(127));
    t1(int16_t(127));
    t1(int32_t(-1));
    t1(numeric_limits<int64_t>::max());

    t2("");
    t2("1");
    t2("11");
    t2("1122333");

    t3();

    return 0;
}












