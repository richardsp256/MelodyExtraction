from pymelex import IntList

def test_IntList():
    l = IntList()
    assert len(l) == 0

    l.append(34)
    assert len(l) == 1
    assert l[0] == 34


    l.append(15)
    assert len(l) == 2
    assert l[0] == 34
    assert l[1] == 15


    l.append(264)
    assert len(l) == 3
    assert l[0] == 34
    assert l[1] == 15
    assert l[2] == 264

    l[1] = -100
    assert len(l) == 3
    assert l[0] == 34
    assert l[1] == -100
    assert l[2] == 264

    l.insert(1,-3432)
    print([e for e in l])
    assert len(l) == 4
    assert l[0] == 34
    assert l[1] == -3432
    assert l[2] == -100
    assert l[3] == 264
